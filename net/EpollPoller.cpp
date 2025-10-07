#include "EpollPoller.h"
#include "Channel.h"
#include "base/Logger.h"
#include <cerrno>
#include <cstring>
#include <unistd.h>

// channel在map中的状态
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),//此处调用父类构造函数
      m_epollfd(::epoll_create1(EPOLL_CLOEXEC)),
      m_events(kInitEventListSize)
{
    if (m_epollfd < 0)
    {
        LOG_FATAL << "epoll_create error:" << errno;
    }
}

EpollPoller::~EpollPoller()
{
    ::close(m_epollfd);
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    int numEvents = ::epoll_wait(m_epollfd, &*m_events.begin(), static_cast<int>(m_events.size()), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == m_events.size())
        {
            m_events.resize(m_events.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG << "timeout!";
    }
    else
    {
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR << "EPollPoller::poll() err!";
        }
    }
    return now;
}

// 先在channel类调整m_events,然后调用这个函数调整底层关注的事件
void EpollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            m_channels[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 物理上的删除,断开连接时调用
void EpollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    m_channels.erase(fd);
    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(m_events[i].data.ptr);
        channel->set_revents(m_events[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel *channel)
{
    
    epoll_event event;
    memset(&event, 0, sizeof event);
    int fd = channel->fd();
    event.events = channel->events();
    event.data.ptr = channel; // 关键：将 Channel 指针存入 epoll_event

    if (::epoll_ctl(m_epollfd, operation, fd, &event) < 0)
    {
        LOG_FATAL << "epoll_ctl error:" << operation;
    }
}