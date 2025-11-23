// net/Timer.h

#ifndef TIMER_H
#define TIMER_H

#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include "Callbacks.h" // 包含 TimerCallback 定义 (需在 Callbacks.h 添加)

#include <atomic>

class Timer : noncopyable
{
public:
    using TimerCallback = std::function<void()>;

    Timer(TimerCallback cb, Timestamp when, double interval)
        : m_callback(std::move(cb)),
          m_expiration(when),
          m_interval(interval),
          m_repeat(interval > 0.0),
          m_sequence(s_numCreated++) // 每一个 Timer 都有唯一的序列号
    {}

    void run() const
    {
        if (m_callback) m_callback();
    }

    Timestamp expiration() const { return m_expiration; }
    bool repeat() const { return m_repeat; }
    int64_t sequence() const { return m_sequence; }

    // 如果是重复定时器，重启它（更新下一次过期时间）
    void restart(Timestamp now);

    static int64_t numCreated() { return s_numCreated; }

private:
    const TimerCallback m_callback; // 定时器回调
    Timestamp m_expiration;         // 下一次的过期时间
    const double m_interval;        // 重复间隔 (0.0 表示一次性)
    const bool m_repeat;            // 是否重复
    const int64_t m_sequence;       // 全局唯一序号

    static std::atomic<int64_t> s_numCreated;
};

#endif