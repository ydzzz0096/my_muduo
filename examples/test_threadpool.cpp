#include "base/ThreadPool.h"
#include "base/Logger.h"
#include <iostream>
#include <unistd.h>

void print_task(int task_id)
{
    LOG_INFO << "Executing task " << task_id;
    sleep(1); // 模拟耗时任务
}

int main()
{
    ThreadPool pool(4, "MyThreadPool");
    pool.start();

    LOG_INFO << "Adding tasks to the pool...";

    // 添加 10 个任务到线程池
    for (int i = 0; i < 10; ++i)
    {
        pool.addTask(std::bind(print_task, i));
    }

    LOG_INFO << "All tasks added. Main thread will sleep for 5 seconds.";
    sleep(5); // 等待一段时间，让线程池有机会执行任务

    LOG_INFO << "Main thread exiting. ThreadPool will be destroyed.";
    // 当 main 结束，pool 对象被析构，所有后台线程应该都能安全退出。

    return 0;
}