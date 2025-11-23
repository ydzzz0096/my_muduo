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
 * 它在构造时创建 socket，并设置 socket 选项。
 * 它不拥有 EventLoop，而是通过指针使用。
 */
class Acceptor : noncopyable
{
public:
    // 定义一个新连接到来的回调函数类型
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
    NewConnectionCallback m_newConnectionCallback; // 上层（TcpServer）设置的回调
    bool m_listening;
};

#endif