#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <string>

// 时间类
class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch); // explicit 防止隐式转换
    
    static Timestamp now();
    std::string toString() const;

private:
    int64_t m_microSecondsSinceEpoch;
};

#endif