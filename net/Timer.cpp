// net/Timer.cpp

#include "Timer.h"

std::atomic<int64_t> Timer::s_numCreated;

void Timer::restart(Timestamp now)
{
    if (m_repeat)
    {
        // 下一次过期时间 = 当前时间 + 间隔
        // 注意：这里简单地用 now + interval，而不是 m_expiration + interval
        // 是为了防止定时器处理由于系统繁忙滞后，导致后续的定时器“堆积”爆发
        m_expiration = addTime(now, m_interval);
    }
    else
    {
        m_expiration = Timestamp(); // 设置为一个无效时间
    }
}