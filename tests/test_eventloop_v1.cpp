// examples/test_eventloop_v1.cpp
#include "net/EventLoop.h"
#include "base/Thread.h"
#include "base/Logger.h"
#include <iostream>
//验证我们 EventLoop 的基本生命周期：它能否在一个独立的线程中启动、运行，并能被另一个线程安全地停止

//充当两个线程之间沟通的“信使”
EventLoop* g_loop;

void thread_func()
{
    g_loop->loop(); // 在新线程里开启事件循环
}

int main()
{
    LOG_INFO << "Main thread started.";
    EventLoop loop;// m_threadId 成员被初始化为 main 线程的 ID
    g_loop = &loop;

    Thread t(thread_func, "EventLoopThread");
    t.start();

    // 主线程等待 5 秒后，退出事件循环
    sleep(5);
    LOG_INFO << "Main thread calling quit...";
    loop.quit();
    
    t.join();

    LOG_INFO << "Main thread exited.";
    return 0;
}