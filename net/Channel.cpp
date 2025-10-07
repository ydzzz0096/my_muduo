#include "Channel.h"
#include "EventLoop.h"
#include "base/Logger.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : m_loop(loop),
      m_fd(fd),
      m_events(0),// 关心事件
      m_revents(0),// 活动事件
      m_index(-1)
{}

Channel::~Channel() {}

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

// 给链接提供的"遥控器",
void Channel::enableReading() { m_events |= kReadEvent; update(); }
void Channel::enableWriting() { m_events |= kWriteEvent; update(); }
void Channel::disableReading() { m_events &= ~kReadEvent; update(); }
void Channel::disableWriting() { m_events &= ~kWriteEvent; update(); }
void Channel::disableAll() { m_events = kNoneEvent; update(); }