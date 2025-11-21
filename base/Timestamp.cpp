// base/Timestamp.cpp

#include "Timestamp.h"
#include <chrono>
#include <ctime>
#include <cstdio> // for snprintf

// 注意：这里不需要写构造函数，因为已经在 .h 文件里写了

Timestamp Timestamp::now()
{
    return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string Timestamp::toString() const
{
    char buf[128] = {0};
    time_t seconds = m_microSecondsSinceEpoch / 1000000;
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