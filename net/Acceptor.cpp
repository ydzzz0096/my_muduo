#include "Acceptor.h"
#include "InetAddress.h"
#include "base/Logger.h"
#include "EventLoop.h" 

#include <sys/types.h>
#include <sys/socket.h>
#include <cerrno>
#include <unistd.h>

// 辅助函数，用于创建一个非阻塞的 socket fd
static int createNonblocking()
{
    // 使用 SOCK_NONBLOCK 和 SOCK_CLOEXEC 标志原子地创建 socket
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_FATAL << "listen socket create err:" << errno;
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : m_loop(loop),
      m_acceptSocket(createNonblocking()), // 创建监听 socket
      m_acceptChannel(loop, m_acceptSocket.fd()),
      m_listening(false)
{
    // 设置 socket 选项，允许地址和端口复用
    m_acceptSocket.setReuseAddr(true);
    m_acceptSocket.setReusePort(reuseport);
    // 绑定地址
    m_acceptSocket.bindAddress(listenAddr);
    
    m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    // 在析构时，确保 Channel 不再监听任何事件，并从 Poller 中移除
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
}

void Acceptor::listen()
{
    // 确保 listen() 只被调用一次，并且在 EventLoop 所在的线程中调用
    m_loop->assertInLoopThread();
    m_listening = true;
    m_acceptSocket.listen();          // 调用底层的 listen 系统调用
    m_acceptChannel.enableReading();  // 将 Channel 注册到 Poller 中，开始监听读事件
}

// listenfd 有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    m_loop->assertInLoopThread();
    InetAddress peerAddr;
    // peerAddr 保存了新连接的地址信息,输出参数
    int connfd = m_acceptSocket.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (m_newConnectionCallback)
        {
            // 如果上层设置了回调，就执行它，将新连接的 fd 和地址信息传递上去
            m_newConnectionCallback(connfd, peerAddr);
        }
        else
        {
            // 如果没有设置回调，则直接关闭新连接
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR << "accept err:" << errno;
        // 这里的错误可能是 fd 耗尽，需要处理
    }
}