// net/TcpClient.cpp

#include "TcpClient.h"
#include "Connector.h"
#include "EventLoop.h"
#include "base/Logger.h"

#include <cstdio> // snprintf
#include <sys/socket.h> // getsockname
#include <cstring> // for memset
#include <cassert> // for assert

namespace 
{
    void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
    {
        loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }

    // 辅助函数：释放 Connector (停止重连)
    void removeConnector(const ConnectorPtr& connector)
    {
        // ... 实际 muduo 中这里可能更复杂，我们简单处理
    }
}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const std::string& nameArg)
    : m_loop(loop),
      m_connector(new Connector(loop, serverAddr)),
      m_name(nameArg),
      m_retry(false),
      m_connect(true),
      m_nextConnId(1)
{
    // 设置 Connector 的回调
    m_connector->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
}

TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << m_name << "] - connector " << m_connector.get();
    TcpConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        conn = m_connection;
    }
    if (conn)
    {
        // 这里的回调必须重置，因为 TcpClient 可能已经销毁了
        CloseCallback cb = std::bind(&::removeConnection, m_loop, std::placeholders::_1);
        m_loop->runInLoop(
            std::bind(&TcpConnection::setCloseCallback, conn, cb));
        
        if (m_loop->isInLoopThread())
        {
            conn->connectDestroyed();
        }
    }
    else
    {
        m_connector->stop();
    }
}

void TcpClient::connect()
{
    LOG_INFO << "TcpClient::connect[" << m_name << "] - connecting to "
             << m_connector->serverAddress().toIpPort();
    m_connect = true;
    m_connector->start();
}

void TcpClient::disconnect()
{
    m_connect = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_connection)
        {
            m_connection->shutdown();
        }
    }
}

void TcpClient::stop()
{
    m_connect = false;
    m_connector->stop();
}

// Connector 连接成功后，回调此函数
void TcpClient::newConnection(int sockfd)
{
    m_loop->assertInLoopThread();
    
    // 1. 获取对端和本地地址
    InetAddress peerAddr(m_connector->serverAddress());
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR << "sockets::getLocalAddr";
    }
    InetAddress localAddr(local);

    // 2. 构造连接名称
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), m_nextConnId);
    ++m_nextConnId;
    std::string connName = m_name + buf;

    // 3. 创建 TcpConnection 对象
    TcpConnectionPtr conn(new TcpConnection(m_loop,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connection = conn;
    }

    // 4. 设置回调
    conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback);
    conn->setWriteCompleteCallback(m_writeCompleteCallback);
    conn->setCloseCallback(
        std::bind(&TcpClient::removeConnection, this, std::placeholders::_1)); // 这里的 removeConnection 是 TcpClient 的成员函数

    // 5. 开启连接
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    m_loop->assertInLoopThread();
    assert(m_loop == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        assert(m_connection == conn);
        m_connection.reset();
    }

    // 这一步至关重要，将 conn 的引用计数 -1，并触发销毁流程
    m_loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    
    if (m_retry && m_connect)
    {
        LOG_INFO << "TcpClient::connect[" << m_name << "] - Reconnecting to "
                 << m_connector->serverAddress().toIpPort();
        m_connector->restart();
    }
}