// net/EventLoop.h

#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "base/noncopyable.h"
#include "base/CurrentThread.h"
#include "base/Timestamp.h"
#include "TimerId.h"
#include "net/Callbacks.h"

#include <functional>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>


class Channel;
class Poller;
class TimerQueue;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    Timestamp pollReturnTime() const { return m_pollReturnTime; }

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    void wakeup();

    // --- 【新增】定时器相关接口 ---
    // 在指定的时间点执行
    TimerId runAt(Timestamp time, TimerCallback cb);
    // 在一段时间后执行
    TimerId runAfter(double delay, TimerCallback cb);
    // 每隔一段时间执行
    TimerId runEvery(double interval, TimerCallback cb);
    // 取消定时器
    void cancel(TimerId timerId);
    // ---------------------------

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
    void handleRead(); 
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool m_looping;
    std::atomic_bool m_quit;
    const pid_t m_threadId;
    Timestamp m_pollReturnTime;

    std::unique_ptr<Poller> m_poller;
    // 【新增】定时器队列管理，EventLoop 拥有它
    std::unique_ptr<TimerQueue> m_timerQueue; 

    int m_wakeupFd;
    std::unique_ptr<Channel> m_wakeupChannel;
    ChannelList m_activeChannels;

    std::atomic_bool m_callingPendingFunctors;
    std::vector<Functor> m_pendingFunctors;
    std::mutex m_mutex;
};

#endif