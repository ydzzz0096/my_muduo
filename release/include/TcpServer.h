#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "base/noncopyable.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <map>

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    // 是否端口复用
    enum Option { kNoReusePort, kReusePort };

    TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,// 首要任务知道自己要建立监听的地址
            const std::string& nameArg,
            Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitcallback(const ThreadInitCallback& cb) { m_threadInitCallback = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { m_connectionCallback = cb; }
    void setMessageCallback(const MessageCallback& cb) { m_messageCallback = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { m_writeCompleteCallback = cb; }

    void setThreadNum(int numThreads);
    void start();

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

    EventLoop* m_loop; // baseLoop 用户定义的 loop
    const std::string m_ipPort;
    const std::string m_name;
    
    std::unique_ptr<Acceptor> m_acceptor;
    std::shared_ptr<EventLoopThreadPool> m_threadPool;
    
    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    WriteCompleteCallback m_writeCompleteCallback;

    ThreadInitCallback m_threadInitCallback;
    std::atomic<int> m_started;
    
    int m_nextConnId;
    ConnectionMap m_connections;
};

#endif