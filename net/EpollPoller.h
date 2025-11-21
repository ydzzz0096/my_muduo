#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H

#include "Poller.h"
#include <vector>
#include <sys/epoll.h>

// Poller 接口的 Linux epoll 具体实现，封装了 epoll_ctl(底层状态修改) 和 epoll_wait(监测结果汇报)，
// 并通过 data.ptr 优化实现了 O(1) 的事件分发。
class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
    
private:
    static const int kInitEventListSize = 16;
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);
    
    using EventList = std::vector<struct epoll_event>;

    int m_epollfd;
    EventList m_events;//用来接收 epoll_wait 系统调用返回的“活跃事件”的数组,因为是活跃所以用来更新revents
};
#endif