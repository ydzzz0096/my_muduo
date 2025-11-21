// net/Connector.cpp

#include "Connector.h"
#include "base/Logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h" // 借用其中的 sockets:: 辅助函数（如果没封装成静态，可能需要直接用系统调用）

#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <cassert>

const int Connector::kMaxRetryDelayMs;
const int Connector::kInitRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : m_loop(loop),
      m_serverAddr(serverAddr),
      m_connect(false),
      m_state(kDisconnected),
      m_retryDelayMs(kInitRetryDelayMs)
{
    LOG_DEBUG << "Connector::ctor[" << this << "]";
}

Connector::~Connector()
{
    LOG_DEBUG << "Connector::dtor[" << this << "]";
    assert(!m_channel);
}

void Connector::start()
{
    m_connect = true;
    m_loop->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    m_loop->assertInLoopThread();
    if (m_connect)
    {
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::stop()
{
    m_connect = false;
    m_loop->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
    m_loop->assertInLoopThread();
    if (m_state == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd); // 这里虽然叫 retry，但其实如果是 stop 状态，retry 不会真的重连
    }
}

void Connector::connect()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_FATAL << "Connector::connect socket creation failed";
    }

    int ret = ::connect(sockfd, (struct sockaddr*)m_serverAddr.getSockAddr(), sizeof(sockaddr_in));
    
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS: // 正在连接（非阻塞模式最常见的情况）
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd); // 可恢复的错误，重试
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_ERROR << "connect error in Connector::startInLoop " << savedErrno;
            ::close(sockfd); // 不可恢复错误，关闭
            break;

        default:
            LOG_ERROR << "Unexpected error in Connector::startInLoop " << savedErrno;
            ::close(sockfd);
            break;
    }
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!m_channel);
    // 创建 Channel 监听该 sockfd
    m_channel.reset(new Channel(m_loop, sockfd));
    
    // 设置回调：连接建立完成时，socket 会变为“可写”
    m_channel->setWriteCallback(std::bind(&Connector::handleWrite, this));
    m_channel->setErrorCallback(std::bind(&Connector::handleError, this));
    
    m_channel->enableWriting(); // 关注 EPOLLOUT
}

int Connector::removeAndResetChannel()
{
    m_channel->disableAll();
    m_channel->remove();
    int sockfd = m_channel->fd();
    
    // 关键：我们不能在这里立刻销毁 Channel，因为我们正在 Channel 的回调函数里！
    // 我们必须把销毁动作推迟到 EventLoop 的下一次循环。
    m_loop->queueInLoop(
        std::bind(&Connector::resetChannel, this)); // resetChannel 负责 m_channel.reset()
        
    return sockfd;
}

// 在 Connector.h 中添加 void resetChannel();
// 在 Connector.cpp 中实现：
void Connector::resetChannel()
{
    m_channel.reset(); // 在这里安全销毁
}


void Connector::handleWrite()
{
    LOG_DEBUG << "Connector::handleWrite " << m_state;

    if (m_state == kConnecting)
    {
        int sockfd = removeAndResetChannel(); // 移除监听，因为我们要么成功了，要么失败重试
        // 我们手动 delete 刚才 release 的 channel，或者刚才不 release 直接 reset
        // 这里为了简单，修正上面的逻辑：
        // 实际上我们应该在这里直接 delete m_channel，但要小心生命周期。
        // 咱们用最简单的方式：
        // int sockfd = m_channel->fd();
        // m_channel->disableAll();
        // m_channel->remove();
        // m_channel.reset(); 
        // 但上面为了解耦，我们还是用 removeAndResetChannel 里的逻辑。
        // 下面这行代码实际上要配合上面的 release 使用，我们这里为了代码跑通，
        // 先假设 removeAndResetChannel 只是 disable 并返回 fd，不 delete。
        // 修正：
        // 为了避免混乱，我们采用简单的：
        // int sockfd = m_channel->fd();
        // m_channel->disableAll();
        // m_channel->remove();
        // m_channel.reset();
        
        // 检查连接是否真的成功
        int err = 0;
        socklen_t len = sizeof err;
        if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
        {
            err = errno;
        }

        if (err)
        {
            // 连接失败
            LOG_ERROR << "Connector::handleWrite - SO_ERROR = " << err << " " << strerror(err);
            retry(sockfd);
        }
        else
        {
            // 连接成功！
            setState(kConnected);
            if (m_connect)
            {
                if (m_newConnectionCallback)
                {
                    m_newConnectionCallback(sockfd); // 回调 TcpClient
                }
                else
                {
                    ::close(sockfd);
                }
            }
            else
            {
                ::close(sockfd); // 期间被 stop 了
            }
        }
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError state=" << m_state;
    if (m_state == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        // 修正：同上，这里应该获取 fd 并 reset channel
        int err = 0;
        socklen_t len = sizeof err;
        ::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);
        LOG_ERROR << "SO_ERROR = " << err << " " << strerror(err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd)
{
    ::close(sockfd); // 关闭旧的 socket
    setState(kDisconnected);

    if (m_connect)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << m_serverAddr.toIpPort()
                 << " in " << m_retryDelayMs << " milliseconds. ";
        
        // 注册定时器进行重连
        m_loop->runAfter(m_retryDelayMs / 1000.0,
                         std::bind(&Connector::startInLoop, shared_from_this()));
        
        // 指数退避算法，延长下次重连时间
        m_retryDelayMs = std::min(m_retryDelayMs * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::restart()
{
    m_loop->assertInLoopThread();
    setState(kDisconnected);
    m_retryDelayMs = kInitRetryDelayMs; // 重置重连间隔为初始值
    m_connect = true;
    startInLoop(); // 立即尝试发起连接
}