// net/EventLoopThread.h (新增文件)

#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include "base/noncopyable.h"
#include "base/Thread.h"
#include <functional>
#include <mutex>
#include <condition_variable>
/*
    将“创建一个新线程”和“在这个新线程里创建并运行一个 EventLoop”这两个紧密相关的操作，“打包”在了一起
    
    提供了一个简洁、安全的接口 (startLoop())，替使用者处理了所有底层的线程创建、EventLoop 对象的构造、以及两者之间的同步问题，确保 EventLoop 对象总是在它自己将要运行的那个线程中被创建
*/
class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunc();

    /*
        threadFunc 能执行完，其实就说明 EventLoopThread 对象正在被销毁，quit() 信号已经发出，这个线程的任务即将结束
    */
    EventLoop* m_loop;// 被子线程拥有,是局部变量存在在栈内,函数执行完就被销毁,这里只提供访问
    bool m_exiting;
    Thread m_thread;// 成员变量和所在类同生命周期
    std::mutex m_mutex;
    std::condition_variable m_cond;
    ThreadInitCallback m_callback;
    /*
        m_callback 是一个由上层用户提供的可选的初始化函数，它由 EventLoopThreadPool 传递给 EventLoopThread，并最终在新的 I/O 线程中、EventLoop::loop() 开始之前被执行，用于对新线程或其 EventLoop 进行自定义的初始化设置
    */
};

#endif