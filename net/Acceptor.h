#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "base/noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>

class EventLoop;
class InetAddress;

/**
 * Acceptor 类的职责是封装服务器端的监听套接字。
 * 它不拥有 EventLoop，而是通过指针使用。
 */
/*
    如果 Acceptor “拥有” EventLoop
    class Acceptor {
    private:
        EventLoop m_loop; // 方式1: 直接作为成员对象
        或者
        std::unique_ptr<EventLoop> m_loop; // 方式2: 通过独占指针
    };
    这意味着 Acceptor 负责 EventLoop 的生命周期,但是实际上Acceptor只是mainloop的一个客户
    Eventloop应该被所在线程拥有,但是Acceptor还是要知道自己的运行环境(包含指针)
*/
class Acceptor : noncopyable
{
public:
    // 定义一个新连接到来的回调函数类型,上层（TcpServer）设置的回调,被 Acceptor 内部的读回调函数 Acceptor::handleRead 来调用
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    // 设置新连接到来的回调函数
    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        m_newConnectionCallback = cb;
    }

    bool listening() const { return m_listening; }
    void listen();

private:
    // 当 m_acceptChannel 上的 fd 有可读事件（新连接）时被调用
    void handleRead();

    EventLoop* m_loop; // Acceptor 所属的 EventLoop
    Socket m_acceptSocket; // 监听套接字
    Channel m_acceptChannel; // 监听套接字对应的 Channel
    NewConnectionCallback m_newConnectionCallback; 
    bool m_listening;
};

#endif