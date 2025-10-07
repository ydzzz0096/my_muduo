// net/EventLoop.h

#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "base/noncopyable.h"
#include "base/CurrentThread.h"
#include "base/Timestamp.h"
#include <functional>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

class Channel;
class Poller;

class EventLoop : noncopyable
{
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    // 【新增】跨线程调用的核心接口
    void runInLoop(std::function<void()> cb);
    void queueInLoop(std::function<void()> cb);

    // 【新增】用于唤醒 loop 所在线程的方法
    void wakeup();

    // EventLoop 与 Poller/Channel 的交互接口
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    bool isInLoopThread() const { return m_threadId == CurrentThread::tid(); }
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }

private:
    void abortNotInLoopThread();
    
    // 【新增】处理唤醒事件和待处理任务的方法
    void handleRead(); // waked up
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool m_looping;
    std::atomic_bool m_quit;
    const pid_t m_threadId;

    std::unique_ptr<Poller> m_poller;
    ChannelList m_activeChannels;// 在EpollPoller类中修改

    // 唤醒机制相关的成员
    int m_wakeupFd;// 门铃
    std::unique_ptr<Channel> m_wakeupChannel;// 哨兵

    // 用于处理跨线程任务的成员
    std::atomic_bool m_callingPendingFunctors;
    std::vector<std::function<void()>> m_pendingFunctors;
    std::mutex m_mutex;
};

#endif