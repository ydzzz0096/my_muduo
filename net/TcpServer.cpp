#include "TcpServer.h"
#include "base/Logger.h"
#include "TcpConnection.h"

#include <functional>
#include <cstring>

TcpServer::TcpServer(EventLoop* loop,
        const InetAddress& listenAddr,
        const std::string& nameArg,
        Option option)
    : m_loop(loop),
      m_ipPort(listenAddr.toIpPort()),
      m_name(nameArg),
      m_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)),
      m_threadPool(new EventLoopThreadPool(loop, m_name)),
      m_connectionCallback(),
      m_messageCallback(),
      m_started(0),
      m_nextConnId(1)
{
    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    // 只需要处理未销毁的连接
    /*
        main 线程绝对不能直接 delete 一个 TcpConnection 对象。因为这个对象内部的 Channel 等资源正被一个 subLoop 线程使用。如果 main 线程销毁了它，subLoop 线程在下一次 poll 返回时就会访问到悬空指针，导致程序崩溃
    */
    for (auto& item : m_connections)
    {
        // 1. 复制一份 shared_ptr,后续在子线程里被摧毁之后,不会立即消失,等到函数结束之后会自动消除
        TcpConnectionPtr conn(item.second);
        
        // 2. 从 map 中移除,如果没有上一步,这里会直接导致连接销毁,子线程里会变成空指针
        item.second.reset();
        
        // 3. 将“销毁”任务派发给 subLoop
        /*
            conn->getLoop():
                TcpConnection 对象在创建时，就“记住”了自己属于哪个 subLoop 线程。
                这个函数返回一个 EventLoop* 指针，指向那个子线程的 EventLoop 实例（我们称之为 subLoop_B）。

            std::bind(&TcpConnection::connectDestroyed, conn):
                打包任务: 我们使用 std::bind，创建了一个新的待办任务 (std::function)。
                这个任务的内容是：“请在 conn 这个对象上，调用 connectDestroyed 方法”。
                关键: std::bind 会再次拷贝 conn 这个 shared_ptr。现在，这个 TcpConnection 对象被这个新的**任务（std::function）**所持有。

            ...->runInLoop( 任务 ):
                主线程 (mainLoop 线程) 调用 subLoop_B 的 runInLoop 方法。
                runInLoop 内部检查发现 isInLoopThread() 为 false（因为现在是主线程在调用子线程的 EventLoop），于是它不会直接执行任务。相反，它会调用 queueInLoop( 任务 )。
        */
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    m_threadPool->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (m_started++ == 0) // 防止一个 TcpServer 对象被 start 多次
    {
        m_threadPool->start(m_threadInitCallback); // 启动底层的 loop 线程池
        m_loop->runInLoop(std::bind(&Acceptor::listen, m_acceptor.get()));
    }
}

// 有一个新的客户端的连接，acceptor 会执行这个回调操作
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 步骤 1: 确认我们在正确的“办公室”（线程）
    m_loop->assertInLoopThread();

    // 步骤 2: 为新员工指派一个“服务部门”（subLoop）
    EventLoop* ioLoop = m_threadPool->getNextLoop();
    
    // 步骤 3: 为新员工制作一个唯一的“工牌”（连接名）
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", m_ipPort.c_str(), m_nextConnId);
    ++m_nextConnId;
    std::string connName = m_name + buf;

    LOG_INFO << "TcpServer::newConnection [" << m_name
        << "] - new connection [" << connName
        << "] from " << peerAddr.toIpPort();

    // 步骤 4: 确认新员工自己的“联系方式”（本地地址）
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR << "sockets::getLocalAddr";
    }
    InetAddress localAddr(local);
    
    // 步骤 5: 创建新员工的“档案”（TcpConnection 对象）
    TcpConnectionPtr conn(new TcpConnection(
                            ioLoop,
                            connName,
                            sockfd,
                            localAddr,
                            peerAddr));
    
    // 步骤 6: 将新员工的档案存入公司的“花名册”
    m_connections[connName] = conn;
    
    // 步骤 7: 为新员工配置“工作职责”和“汇报制度”（设置回调）
    conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback);
    conn->setWriteCompleteCallback(m_writeCompleteCallback);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 步骤 8: 通知新员工去他被分配的“服务部门”报到并开始工作
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

/*
    主线程和子线程没有数据竞争:
    TcpServer (在 mainLoop) 负责从 map 中“除名”。
    TcpConnection (在 subLoop) 负责清理自己的 Channel 和 Poller所以并不会产生数据竞争
    操作不同的数据
    
    子线程内部绝对不会有数据竞争:
    这是因为 subLoop (它是一个 EventLoop) 本身，就是一个天然的“任务串行化执行器” (Task Serializer)。它保证了所有派发给它的任务，都会排队并一个接一个地执行，永远不会并发。
*/
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    m_loop->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    m_loop->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << m_name
        << "] - connection " << conn->name();
    
    m_connections.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}