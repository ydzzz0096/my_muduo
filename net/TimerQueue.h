// net/TimerQueue.h

#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H

#include <set>
#include <vector>
#include <memory>

#include "base/Timestamp.h"
#include "base/noncopyable.h"
#include "Channel.h"

class EventLoop;
class Timer;
class TimerId;

/**
 * @brief 定时器队列，基于 timerfd 实现。
 * 这是一个内部类，不应该暴露给用户。
 * 它持有 Timer 列表，并负责管理 timerfd，当最早的定时器到期时触发 handleRead。
 */
class TimerQueue : noncopyable
{
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    // 插入定时器 (由 EventLoop 线程调用)
    // 必须是线程安全的，通常由 EventLoop::runInLoop 调用
    TimerId addTimer(std::function<void()> cb, Timestamp when, double interval);

    // 取消定时器
    void cancel(TimerId timerId);

private:
    // timerfd 变为可读时的回调
    void handleRead();
    
    // 核心逻辑：获取所有已过期的定时器
    std::vector<Timer*> getExpired(Timestamp now);
    
    // 核心逻辑：重置这些定时器（如果是重复的则再次添加，否则销毁）
    void reset(const std::vector<Timer*>& expired, Timestamp now);

    void cancelInLoop(TimerId timerId);
    
    // 插入定时器的内部实现
    bool insert(Timer* timer);

    EventLoop* m_loop;
    const int m_timerfd;        // Linux 特有的定时器文件描述符
    Channel m_timerfdChannel;   // 用于监听 timerfd 的 Channel
    
    // Timer list sorted by expiration
    // 使用 pair<Timestamp, Timer*> 是为了处理两个定时器到期时间完全相同的情况
    // 此时通过 Timer* 的地址大小来区分，保证 set 中元素的唯一性
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;
    
    // 同样用于保存 Timer，用于 cancel 时的查找
    using ActiveTimer = std::pair<Timer*, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    TimerList m_timers;          // 按时间排序的主列表
    ActiveTimerSet m_activeTimers; // 按对象地址排序的辅助列表（用于删除）
    
    // 标识是否正在处理过期定时器（用于处理在回调中把自己 cancel 掉的边缘情况）
    bool m_callingExpiredTimers; 
    ActiveTimerSet m_cancelingTimers; // 保存正在取消的定时器
};

#endif