#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <string>

// 时间类
class Timestamp
{
public:
    Timestamp();
    // 保证跨平台的精确位数,确定的64位数
    // long的位数不确定
    explicit Timestamp(int64_t microSecondsSinceEpoch); // explicit 防止隐式转换
    
    static Timestamp now();
    std::string toString() const;

private:
    int64_t m_microSecondsSinceEpoch;
};

#endif