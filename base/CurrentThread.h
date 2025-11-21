// base/CurrentThread.h

#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

#include <unistd.h>//Linux/Unix 系统编程的基础，它提供了与操作系统底层功能交互所必需的大量接口。包含系统调用syscall()
#include <sys/syscall.h>//不是为了声明 syscall() 函数本身（它通常在 unistd.h 里），而是为了获得 SYS_gettid 这个宏定义

namespace CurrentThread
{
    // extern表示这只是一个声明,而不是定义
    // __thread 是一个 GCC 的关键字，用于实现线程局部存储
    // 它可以保证每个线程都拥有一个自己独立的 t_cachedTid 变量副本
    extern __thread int t_cachedTid;

    void cacheTid();


    // inline: 性能提升,建议原地展开函数
    //         .h中实现函数体必须用inline修饰
    inline int tid()
    {
        // __builtin_expect 是 GCC 的一个优化提示，告诉编译器我们期望这个 if 条件很少为真
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}

#endif