#ifndef POLLER_H
#define POLLER_H

#include "base/Timestamp.h"
// 应该唯一掌管唯一的epoll实例,不能被复制
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
    /*
    virtual
    作用: virtual 关键字用于声明一个虚函数 (Virtual Function)
    核心目的: 启用多态 (Polymorphism)，特别是运行时多态 (Runtime Polymorphism)
    = 0
    有一个纯虚函数则是抽象类:不能被实例化,必须被继承,强制子类实现
    */
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    virtual bool hasChannel(Channel *channel) const;
    static Poller *newDefaultPoller(EventLoop *loop);
    void assertInLoopThread() const;

    // 区分static,子类会继承这个变量,但是 不是共享同一个,会自己创造一份
protected:
    using ChannelMap = std::map<int, Channel *>; // fd和Channel的映射
    ChannelMap m_channels;

    // 子类构造的过程中被初始化
    // 为所有派生类提供接口,不直接访问,但是间接使用
private:
    EventLoop *m_ownerLoop;// 为了确定自己的工作环境,确定thread
};
#endif