// net/TcpConnection.cpp

#include "TcpConnection.h"
#include "base/Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <cerrno>
#include <functional>
#include <string>

static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "mainLoop is null!";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
            const std::string& name,
            int sockfd,
            const InetAddress& localAddr,
            const InetAddress& peerAddr)
    : m_loop(CheckLoopNotNull(loop)),
      m_name(name),
      m_state(kConnecting),
      m_reading(true),
      m_socket(new Socket(sockfd)),
      m_channel(new Channel(loop, sockfd)),
      m_localAddr(localAddr),
      m_peerAddr(peerAddr),
      // 流量控制,高水位线
      m_highWaterMark(64 * 1024 * 1024) // 64M
{
    // 下面给 channel 设置相应的回调函数，poller 给 channel 通知感兴趣的事件发生了，channel 会回调相应的操作函数
    m_channel->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));// 留一个空位接收参数
    m_channel->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    m_channel->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    m_channel->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO << "TcpConnection::ctor[" << m_name << "] at " << this
        << " fd=" << sockfd;
    m_socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO << "TcpConnection::dtor[" << m_name << "] at " << this
        << " fd=" << m_channel->fd()
        << " state=" << (int)m_state;
}

void TcpConnection::send(const std::string& buf)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            m_loop->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

/**
 * 发送数据  应用写的快， 而内核发送数据慢， 需要把待发送数据写入缓冲区， 而且设置了水位线防止发的太快
 */
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    m_loop->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该 connection 的 shutdown，不能再进行发送了
    if (m_state == kDisconnected)
    {
        LOG_ERROR << "disconnected, give up writing!";
        return;
    }

    // 表示 channel_ 第一次开始写数据，而且缓冲区没有待发数据
    if (!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
    {
        nwrote = ::write(m_channel->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && m_writeCompleteCallback)
            {
                // 既然在这里数据全部发送完成，就不用再给 channel 设置 epollout 事件了
                m_loop->queueInLoop(
                    std::bind(m_writeCompleteCallback, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE  RESET
                {
                    faultError = true;
                }
            }
        }
    }

    // 说明当前这一次 write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给 channel
    // 注册 epollout 事件，poller 发现 tcp 的发送缓冲区有空间，会通知相应的 sock-channel，调用 writeCallback_
    // 也就是调用 TcpConnection::handleWrite 方法，把发送缓冲区中的数据全部发送完成
    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen = m_outputBuffer.readableBytes();
        if (oldLen + remaining >= m_highWaterMark && oldLen < m_highWaterMark && m_highWaterMarkCallback)
        {
            m_loop->queueInLoop(
                std::bind(m_highWaterMarkCallback, shared_from_this(), oldLen + remaining));
        }
        m_outputBuffer.append((char*)data + nwrote, remaining);
        if (!m_channel->isWriting())
        {
            m_channel->enableWriting(); // 这里一定要注册 channel 的写事件，否则 poller 不会给 channel 通知 epollout
        }
    }
}

void TcpConnection::shutdown()
{
    if (m_state == kConnected)
    {
        setState(kDisconnecting);
        m_loop->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    m_loop->assertInLoopThread();
    if (!m_channel->isWriting()) // 说明 outputBuffer 中的数据已经全部发送完成
    {
        m_socket->shutdownWrite(); // 关闭写端
    }
}


void TcpConnection::connectEstablished()
{
    m_loop->assertInLoopThread();
    setState(kConnected);
    m_channel->tie(shared_from_this());
    m_channel->enableReading(); // 向 poller 注册 channel 的 epollin 事件

    // 新连接建立，执行回调
    m_connectionCallback(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    m_loop->assertInLoopThread();
    if (m_state == kConnected)
    {
        setState(kDisconnected);
        m_channel->disableAll(); // 把 channel 的所有感兴趣的事件，从 poller 中 del 掉
        m_connectionCallback(shared_from_this());
    }
    m_channel->remove(); // 把 channel 从 poller 的 ChannelMap 中删除掉
}


void TcpConnection::handleRead(Timestamp receiveTime)
{
    m_loop->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = m_inputBuffer.readFd(m_channel->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作 onMessage
        m_messageCallback(shared_from_this(), &m_inputBuffer, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    m_loop->assertInLoopThread();
    if (m_channel->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = m_outputBuffer.writeFd(m_channel->fd(), &savedErrno);
        if (n > 0)
        {
            m_outputBuffer.retrieve(n);
            if (m_outputBuffer.readableBytes() == 0)
            {
                m_channel->disableWriting();
                if (m_writeCompleteCallback)
                {
                    m_loop->queueInLoop(
                        std::bind(m_writeCompleteCallback, shared_from_this()));
                }
                if (m_state == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR << "TcpConnection::handleWrite";
        }
    }
    else
    {
        LOG_ERROR << "TcpConnection fd=" << m_channel->fd()
            << " is down, no more writing";
    }
}

// poller => channel::closeCallback => TcpConnection::handleClose
void TcpConnection::handleClose()
{
    m_loop->assertInLoopThread();
    LOG_INFO << "fd=" << m_channel->fd() << " state=" << (int)m_state;
    setState(kDisconnected);
    m_channel->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    m_connectionCallback(connPtr); // 执行连接关闭的回调
    m_closeCallback(connPtr); // 关闭连接的回调 执行的是 TcpServer::removeConnection 回调方法
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(m_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR << "TcpConnection::handleError name:" << m_name
        << " - SO_ERROR:" << err;
}