#include "net/EventLoop.h"
#include "base/Logger.h"
#include "base/Timestamp.h"

#include <iostream>

EventLoop* g_loop;

void print(const char* msg)
{
    LOG_INFO << msg << " at " << Timestamp::now().toString();
}

void cancel_test(TimerId id)
{
    LOG_INFO << "Cancel timer!";
    g_loop->cancel(id);
}

int main()
{
    EventLoop loop;
    g_loop = &loop;

    LOG_INFO << "Start at " << Timestamp::now().toString();

    // 1. 3秒后执行一次
    loop.runAfter(3.0, std::bind(print, "once3"));
    
    // 2. 2秒后执行一次
    loop.runAfter(2.0, std::bind(print, "once2"));
    
    // 3. 4.5秒后执行一次
    loop.runAfter(4.5, std::bind(print, "once4.5"));

    // 4. 每秒执行一次 (重复)
    TimerId id = loop.runEvery(1.0, std::bind(print, "every1"));

    // 5. 5秒后取消那个重复的任务
    loop.runAfter(5.0, std::bind(cancel_test, id));

    // 6. 10秒后退出主循环
    loop.runAfter(10.0, std::bind(&EventLoop::quit, &loop));

    loop.loop();
    
    LOG_INFO << "Main Loop exits";
    return 0;
}