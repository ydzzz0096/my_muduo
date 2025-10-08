// net/Socket.cpp

#include "Socket.h"
#include "InetAddress.h"
#include "base/Logger.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cstring>

Socket::~Socket()
{
    ::close(m_sockfd);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
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
    // 第二个第三个参数都是输出参数
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