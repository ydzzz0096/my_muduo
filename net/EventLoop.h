// net/EventLoop.h
#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "base/noncopyable.h"
#include "base/CurrentThread.h"
#include <atomic>
#include <mutex>

// 事件循环类
class EventLoop : noncopyable
{
public:
    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    // 判断 EventLoop 对象是否在自己的线程里
    // 在类定义内部实现的函数都是“隐式内联”的
    bool isInLoopThread() const { return m_threadId == CurrentThread::tid(); }
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }

private:
    void abortNotInLoopThread();

    std::atomic_bool m_looping;
    std::atomic_bool m_quit;
    const pid_t m_threadId; // 记录当前 EventLoop 所在线程的 id
};
#endif