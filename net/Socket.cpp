// net/Socket.cpp

#include "Socket.h"
#include "InetAddress.h"
#include "base/Logger.h"

#include <unistd.h>
#include <sys/types.h>
/*
    size_t: 表示大小的无符号整数。
    ssize_t: 表示可能返回错误（-1）的大小，read/write 的返回值类型就是它。
    pid_t: 进程 ID 类型。
    socklen_t: socket API 中用于表示 sockaddr 结构体长度的专用类型。
*/
#include <sys/socket.h>// 核心 socket 函数,核心 socket 常量
#include <netinet/tcp.h>// 专门提供与 TCP 协议相关的宏定义和选项。
#include <cstring>

Socket::~Socket()
{
    ::close(m_sockfd);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    // 给一个 socket 文件描述符（sockfd）“绑定”上一个具体的地址（addr）
    if (0 != ::bind(m_sockfd, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL << "bind sockfd:" << m_sockfd << " fail.";
    }
}

void Socket::listen()
{
    // 1024 是 backlog 的大小:最大连接申请数
    if (0 != ::listen(m_sockfd, 1024))
    {
        LOG_FATAL << "listen sockfd:" << m_sockfd << " fail.";
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));

    // 使用 accept4 可以原子地设置新连接 socket 的 NONBLOCK 和 CLOEXEC 标志
    // 第二个是输出参数,accept从全连接队列里取出一个连接,把连接对应的ip地址返回给addr
    // m_sockfd 代表的是那个**“监听套接字” (Listening Socket)**，也就是 listenfd
    int connfd = ::accept4(m_sockfd, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    else
    {
        LOG_ERROR << "accept error:" << errno;
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if (::shutdown(m_sockfd, SHUT_WR) < 0)
    {
        LOG_ERROR << "shutdownWrite error";
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}