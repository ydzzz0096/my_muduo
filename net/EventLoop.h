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
// 亮点: 跨线程安全 唤醒机制
class EventLoop : noncopyable
{
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    // 跨线程调用的核心接口
    void runInLoop(std::function<void()> cb);
    void queueInLoop(std::function<void()> cb);

    // 用于唤醒 loop 所在线程的方法
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
    
    // 处理唤醒事件和待处理任务的方法
    void handleRead(); 
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool m_looping;
    std::atomic_bool m_quit;
    const pid_t m_threadId;

    std::unique_ptr<Poller> m_poller;
    ChannelList m_activeChannels;// 由EpollPoller类更改后上报

    // 唤醒机制相关的成员
    int m_wakeupFd;// 门铃 write() (按门铃): 当外部线程调用 EventLoop::wakeup() 并向 m_wakeupFd 写入一个 uint64_t 的 1 时，内核中的计数器会加 1
    std::unique_ptr<Channel> m_wakeupChannel;// 哨兵 唯一负责的对象就是 m_wakeupFd。如果‘门铃’响了（可读事件发生），（m_wakeupChannel）的应急预案就是调用我(EventLoop) 的 handleRead 方法。

    // 用于处理跨线程任务的成员
    std::atomic_bool m_callingPendingFunctors;
    /*这个标志位用来指示 EventLoop 线程当前是否正在执行 doPendingFunctors() 函数（即，是否正在处理上一批次的待办事项）.防止EventLoop 线程执行完当前的 doPendingFunctors 后，如果没有被 wakeup()，它可能会立刻回到 poll() 并再次阻塞，导致那个刚刚被添加的新任务要等到下一次 I/O 事件或超时才能被执行*/
    std::vector<std::function<void()>> m_pendingFunctors;// 存放跨线程任务的列表
    std::mutex m_mutex;
};

#endif