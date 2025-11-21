// base/noncopyable.h

#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

    //禁止外部实例化: 外部代码无法访问 protected 的构造函数，因此 noncopyable nc; 会编译失败，保证了它只能被继承
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif