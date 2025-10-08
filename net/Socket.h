// net/Socket.h

#ifndef SOCKET_H
#define SOCKET_H

#include "base/noncopyable.h"

class InetAddress; // 前向声明

// 封装 socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : m_sockfd(sockfd)
    {}

    ~Socket();

    int fd() const { return m_sockfd; }

    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

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