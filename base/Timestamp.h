// base/Timestamp.h

#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <iostream>
#include <string>
#include <cstdint> // for int64_t

class Timestamp
{
public:
    explicit Timestamp(int64_t microSecondsSinceEpochArg = 0)
        : m_microSecondsSinceEpoch(microSecondsSinceEpochArg)
    {}

    static Timestamp now();
    std::string toString() const;

    // 获取微秒数
    int64_t microSecondsSinceEpoch() const { return m_microSecondsSinceEpoch; }
    
    // 【新增】判断是否有效
    bool valid() const { return m_microSecondsSinceEpoch > 0; }

    // 【新增】一些常用的时间转换常量
    static const int kMicroSecondsPerSecond = 1000 * 1000;

    // 【新增】比较运算符重载 (作为成员函数或非成员函数均可，这里推荐非成员)
    bool operator<(const Timestamp& rhs) const
    {
        return this->m_microSecondsSinceEpoch < rhs.m_microSecondsSinceEpoch;
    }

    bool operator==(const Timestamp& rhs) const
    {
        return this->m_microSecondsSinceEpoch == rhs.m_microSecondsSinceEpoch;
    }

private:
    int64_t m_microSecondsSinceEpoch;
};

// 【新增】为了让 Timestamp 能支持像 timestamp1 < timestamp2 这样的操作
// 也可以选择把上面的成员函数移出来定义在这里，或者保留上面的成员函数形式。
// 只要有 operator< 就行。上面的成员函数写法已经足够解决编译错误。

// 全局辅助函数声明 (之前已经添加过)
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

#endif