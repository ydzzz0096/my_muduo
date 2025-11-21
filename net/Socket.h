// net/Socket.h

#ifndef SOCKET_H
#define SOCKET_H

#include "base/noncopyable.h"

class InetAddress; // 前向声明

/*
    Socket 类的设计，并不是一个“Socket 工厂”，而是一个“Socket 管家”
    Socket 类的存在，是为了实现 RAII：它接管一个 fd 的所有权，并承诺在这个 Socket 对象生命周期结束时，一定会 close 这个 fd，防止资源泄漏
    分离两个职责:资源的创建和管理
*/
/*
    Socket 类现在可以用来封装任何 socket fd，而不仅仅是新创建的。
    
    最重要的例子：TcpServer 在 accept 成功后，会得到一个 connfd。这个 connfd 不是由 ::socket() 创建的，而是由 ::accept4() 返回的。TcpServer 接下来会 new TcpConnection，而 TcpConnection 内部也会创建一个 Socket 对象，来管理这个 connfd。
    如果 Socket 类的构造函数写死了，只能调用 ::socket()，那么它就无法被 TcpConnection 用来封装 connfd 了。
*/
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : m_sockfd(sockfd)//接收一个已经创建好的sockfd
    {}

    ~Socket();

    int fd() const { return m_sockfd; }

    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    // shutdownWrite() 是 TcpConnection 类实现优雅关闭逻辑的核心
    // 相当于服务端发送了FIN报文,而不是直接断开连接
    void shutdownWrite();

    // 以下是设置 socket 选项的函数
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
    
private:
    const int m_sockfd;
};

#endif