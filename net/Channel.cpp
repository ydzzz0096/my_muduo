#include "Channel.h"
#include "EventLoop.h"
#include "base/Logger.h"
#include <sys/epoll.h>

// 会出现的读写事件,实际上是一串二进制数
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : m_loop(loop),
      m_fd(fd),
      m_events(0),// 关心事件
      m_revents(0),// 活动事件
      m_index(-1),
      m_tied(false)
{}

Channel::~Channel() {}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    m_tie = obj; // 用 weak_ptr 观察 obj
    m_tied = true;
}

// 实现封装,提供单一修改点
void Channel::update()
{
    // 通过 Channel 所属的 EventLoop, 调用 Poller 的相应方法，注册 fd 的 events 事件
    m_loop->updateChannel(this);
}

void Channel::remove()
{
    m_loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    // 如果已经绑定了对象
    if (m_tied)
    {
        // 尝试将 weak_ptr 提升为 shared_ptr
        std::shared_ptr<void> guard = m_tie.lock();
        if (guard) // 提升成功，说明对象还活着
        {
            handleEventWithGuard(receiveTime);
        }
        // 如果提升失败，说明对象已经销毁，什么也不做
    }
    else // 如果没有绑定，直接处理
    {
        handleEventWithGuard(receiveTime);
    }
}

// 【新增】真正处理事件的函数
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    if ((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN))
    {
        if (m_closeCallback) { m_closeCallback(); }
    }
    if (m_revents & EPOLLERR)
    {
        if (m_errorCallback) { m_errorCallback(); }
    }
    if (m_revents & (EPOLLIN | EPOLLPRI))
    {
        if (m_readCallback) { m_readCallback(receiveTime); }
    }
    if (m_revents & EPOLLOUT)
    {
        if (m_writeCallback) { m_writeCallback(); }
    }
}

// Channel 提供给其直接所有者（Acceptor, TcpConnection, EventLoop）的控制接口
void Channel::enableReading() { m_events |= kReadEvent; update(); }
void Channel::enableWriting() { m_events |= kWriteEvent; update(); }
void Channel::disableReading() { m_events &= ~kReadEvent; update(); }
void Channel::disableWriting() { m_events &= ~kWriteEvent; update(); }
void Channel::disableAll() { m_events = kNoneEvent; update(); }