#ifndef POLLER_H
#define POLLER_H

#include "base/Timestamp.h"
// 应该唯一掌管唯一的epoll实例
#include "base/noncopyable.h"
#include <map>
#include <vector>

class Channel;
class EventLoop;

class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    virtual bool hasChannel(Channel *channel) const;
    static Poller *newDefaultPoller(EventLoop *loop);

    // 区分static,子类会继承这个变量,但是不是共享同一个,会自己创造一份
protected:
    using ChannelMap = std::map<int, Channel *>; // fd和Channel的映射
    ChannelMap m_channels;

    // 子类构造的过程中被初始化
    // 为所有派生类提供统一的服务
private:
    EventLoop *m_ownerLoop;// 为了确定自己的工作环境,确定thread
};
#endif