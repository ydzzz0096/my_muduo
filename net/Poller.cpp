#include "Poller.h"
#include "Channel.h"
#include "EpollPoller.h" // 需要包含具体实现

Poller::Poller(EventLoop* loop)
    : m_ownerLoop(loop)
{}

// 提供通用实现
bool Poller::hasChannel(Channel* channel) const
{
    auto it = m_channels.find(channel->fd());
    return it != m_channels.end() && it->second == channel;
}

// 这个工厂方法，是解耦的关键
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    // 实际项目中，这里可以用 #ifdef 来判断平台，选择 epoll 或 kqueue
    return new EpollPoller(loop);
}