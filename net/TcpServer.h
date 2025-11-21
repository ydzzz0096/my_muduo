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

    /*
        这就是处于最上层的接口了,应该由用户设定然后在封装内层层传递
        setMessageCallback:
            经历EchoServer->TcpServer->TcpConnection->Channel. TcpConnection 不会把 onMessage 回调直接给 Channel。它会把自己的成员函数 handleRead 设置给 Channel 只知道“读事件发生时，要调用 TcpConnection::handleRead”
            最终的调用链: EventLoop 调 Channel::handleEvent -> 调 TcpConnection::handleRead -> handleRead 内部再调 m_messageCallback
        ConnectionCallback(连接状态改变回调):
            EchoServer -> TcpServer: server.setConnectionCallback(onConnection)。TcpServer 存入 m_connectionCallback。
            TcpServer -> TcpConnection: conn->setConnectionCallback(m_connectionCallback)。TcpConnection 存入 m_connectionCallback。
            最终调用: TcpConnection 在 connectEstablished（连接建立）和 handleClose（连接关闭）时，自己去调用它持有的 m_connectionCallback。
        WriteCompleteCallback:
            User  → TcpServer  → TcpConnection
            最终调用: TcpConnection 在 handleWrite 中发现 outputBuffer 被清空时，自己去调用它持有的 m_writeCompleteCallback
        
        setThreadInitCallback:
            User (定义者) → TcpServer (保存者) → EventLoopThreadPool (传递者) → EventLoopThread (最终持有者和调用者)
            最终调用: EventLoopThread::threadFunc（在新线程中）创建 EventLoop 后，整合到threadFunc中调用 m_callback(&loop)
    */
    void setThreadInitcallback(const ThreadInitCallback& cb) { m_threadInitCallback = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { m_connectionCallback = cb; }
    void setMessageCallback(const MessageCallback& cb) { m_messageCallback = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { m_writeCompleteCallback = cb; }

    void setThreadNum(int numThreads);
    void start();

private:
    /*
        setConnectionCallback: 是一个**“公共 API”** (Public API)，由库的使用者（EchoServer）调用，代表想要在“连接建立时”或“连接断开时”执行的具体业务逻辑。在 EchoServer 的例子中，这个 cb 就是您的 onConnection 函数（即打印 Connection UP/DOWN 日志）
        newConnection: 是一个**“私有回调”** (Internal Callback)，由库的内部组件（Acceptor）调用，用来**“执行”**连接建立的流程。执行一系列“新员工入职”的内部流程被注册给了 Acceptor 作为 Acceptor 的 NewConnectionCallback。所以，它最终是由 Acceptor::handleRead() 回调的
    */
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

    EventLoop* m_loop; // baseLoop 用户定义的 loop
    const std::string m_ipPort;
    const std::string m_name;
    
    // 直接和这个上层类接触的只有Acceptor和EventLoopThreadPool两个类
    // 从 m_acceptor 接收新连接，然后交给 m_threadPool 去处理
    std::unique_ptr<Acceptor> m_acceptor;
    std::shared_ptr<EventLoopThreadPool> m_threadPool;
    
    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    WriteCompleteCallback m_writeCompleteCallback;
    ThreadInitCallback m_threadInitCallback;
    
    std::atomic<int> m_started;
    int m_nextConnId;
    
    //为了在不使用锁的情况下，保证 m_connections 的线程安全,新增和删除都应该在主线程进行
    // 为什么 EventLoopThreadPool 不能保存？EventLoopThreadPool（人事部）的职责是管理“员工” (EventLoopThread)即thread和Eventloop
    ConnectionMap m_connections;
};

#endif