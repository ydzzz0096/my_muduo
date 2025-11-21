#include "Timestamp.h"
#include <chrono>// 负责高精度地获取当前时间点
#include <ctime>// 负责将这个时间点格式化成我们熟悉的年月日时分秒字符串

Timestamp::Timestamp() : m_microSecondsSinceEpoch(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : m_microSecondsSinceEpoch(microSecondsSinceEpoch)
{
}

Timestamp Timestamp::now()
{
    // 获取当前时间，并把它表示为“从 Unix Epoch (1970年1月1日) 到现在所经过的总微秒数”，最后用这个数字创建一个 Timestamp 对象
    // now返回一个time_point
    // time_since_epoch返回一个duration,但是单位不确定
    // count返回一个纯数字
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