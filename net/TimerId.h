// net/TimerId.h

#ifndef TIMERID_H
#define TIMERID_H
#include <cstdint>
class Timer;

// 用户持有的定时器 ID，用于取消定时器
class TimerId
{
public:
    TimerId()
        : m_timer(nullptr),
          m_sequence(0)
    {}

    TimerId(Timer* timer, int64_t seq)
        : m_timer(timer),
          m_sequence(seq)
    {}

    // 主要是为了能让 TimerQueue 知道要取消哪一个
    friend class TimerQueue;

private:
    Timer* m_timer;
    int64_t m_sequence;
};

#endif