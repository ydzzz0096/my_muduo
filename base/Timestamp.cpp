#include "Timestamp.h"
#include <chrono>
#include <ctime>

Timestamp::Timestamp() : m_microSecondsSinceEpoch(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : m_microSecondsSinceEpoch(microSecondsSinceEpoch)
{
}

Timestamp Timestamp::now()
{
    // 使用 C++ 的 <chrono> 库来获取当前时间，更精确
    return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string Timestamp::toString() const
{
    char buf[128] = {0};
    time_t seconds = m_microSecondsSinceEpoch / 1000000;
    
    // 使用 localtime_r 是线程安全的版本，比 localtime 更好
    tm tm_time;
    localtime_r(&seconds, &tm_time);

    snprintf(buf, 128, "%04d-%02d-%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec);
    return buf;
}