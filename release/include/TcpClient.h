// net/TcpClient.h

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "base/noncopyable.h"
#include "TcpConnection.h"

#include <mutex>
#include <atomic>

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable
{
public:
    TcpClient(EventLoop* loop,
              const InetAddress& serverAddr,
              const std::string& nameArg);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_connection;
    }

    EventLoop* getLoop() const { return m_loop; }
    bool retry() const { return m_retry; }
    void enableRetry() { m_retry = true; }

    const std::string& name() const { return m_name; }

    // 设置回调函数 (与 TcpServer 类似)
    void setConnectionCallback(ConnectionCallback cb) { m_connectionCallback = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { m_messageCallback = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { m_writeCompleteCallback = std::move(cb); }

private:
    // Connector 连接成功时的回调
    void newConnection(int sockfd);
    // TcpConnection 断开时的回调
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* m_loop;
    ConnectorPtr m_connector; // 负责发起连接的组件
    const std::string m_name;
    
    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    WriteCompleteCallback m_writeCompleteCallback;

    std::atomic_bool m_retry;   // 是否自动重连
    std::atomic_bool m_connect; // 是否正在连接
    
    // always in loop thread
    int m_nextConnId;
    mutable std::mutex m_mutex;
    TcpConnectionPtr m_connection; // 客户端通常只有一个连接
};

#endif