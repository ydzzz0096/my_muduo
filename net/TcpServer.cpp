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
    for (auto& item : m_connections)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
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