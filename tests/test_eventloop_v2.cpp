// examples/test_eventloop_v2.cpp

#include "net/EventLoop.h"
#include "base/Thread.h"
#include "base/Logger.h"

#include <iostream>
#include <unistd.h>
#include <mutex>
#include <condition_variable>

// 全局变量，用于主线程和子线程之间的同步
EventLoop* g_loop = nullptr;
std::mutex g_mutex;
std::condition_variable g_cond;

// 子线程要执行的函数
void thread_func()
{
    LOG_INFO << "EventLoop thread started.";

    // 1. 在子线程中创建 EventLoop 对象
    EventLoop loop;

    // 2. EventLoop 创建完成后，通知主线程
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_loop = &loop;
        g_cond.notify_one();
    }

    // 3. 开始事件循环
    loop.loop();
    
    // loop 结束后，g_loop 可能会悬空，安全起见置空
    std::lock_guard<std::mutex> lock(g_mutex);
    g_loop = nullptr;
}

int main()
{
    LOG_INFO << "Main thread started.";
    
    // 4. 创建子线程
    Thread t(thread_func, "EventLoopThread");
    t.start();
    
    EventLoop* loopInOtherThread = nullptr;
    // 5. 主线程阻塞等待，直到子线程通知它 EventLoop 已经创建完毕
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        // wait 函数会等待，直到 g_loop 被赋值
        g_cond.wait(lock, [&]{ return g_loop != nullptr; });
        loopInOtherThread = g_loop;
    }

    // 6. 主线程向子线程的 EventLoop 派发一个任务
    LOG_INFO << "Main thread queuing a task...";
    loopInOtherThread->queueInLoop([](){
        LOG_INFO << "Task executed in EventLoop thread.";
    });

    // 7. 主线程等待 3 秒后，请求 EventLoop 退出
    sleep(3);
    LOG_INFO << "Main thread calling quit...";
    if (loopInOtherThread)
    {
        loopInOtherThread->quit();
    }
    
    t.join();

    LOG_INFO << "Main thread exited.";
    return 0;
}