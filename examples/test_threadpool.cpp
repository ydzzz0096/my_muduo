#include "base/ThreadPool.h"
#include "base/Logger.h"
#include <iostream>
#include <unistd.h>//usleep函数
#include <atomic> // 引入 atomic 用于更精确的测试
#include <cassert> // 引入 assert 用于断言

void print_task(int task_id, std::atomic<int>& completed_counter)
{
     //LOG_INFO << "Executing task " << task_id;
     //usleep(100 * 1000); // 模拟 100ms 的任务
    completed_counter++; // 任务完成时，计数器加1
}

int main()
{
    ThreadPool pool(4, "MyThreadPool");
    pool.start();

    const int TASK_COUNT = 100000; // 测试更多的任务
    std::atomic<int> completed_tasks(0); // 创建一个原子计数器

    LOG_INFO << "Adding " << TASK_COUNT << " tasks to the pool...";

    for (int i = 0; i < TASK_COUNT; ++i)
    {
        // 我们让任务在完成时增加计数器
        pool.addTask([i, &completed_tasks]() { 
            print_task(i, completed_tasks); 
        });
    }

    LOG_INFO << "All tasks added. Explicitly shutting down the pool.";
    
    // 这是测试的关键！主动、显式地关闭线程池。
    // 这个函数应该是阻塞的，直到所有100个任务都执行完毕，它才会返回。
    pool.shutdown();

    LOG_INFO << "ThreadPool has been shut down.";

    // **最终的黄金标准验证**
    // 我们可以断言，所有提交的任务都必须已经被完成了。
    LOG_INFO << "Completed " << completed_tasks << " tasks.";
    assert(completed_tasks == TASK_COUNT);

    return 0;
}