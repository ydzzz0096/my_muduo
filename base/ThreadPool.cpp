// base/ThreadPool.cpp

#include "ThreadPool.h"
#include "Logger.h"

ThreadPool::ThreadPool(int threadNum, const std::string& name)
    : m_name(name),
      m_threadNum(threadNum),
      m_started(false)
{}

// 【核心】修改析构函数，实现优雅关闭
ThreadPool::~ThreadPool()
{
    if (m_started)
    {
        // 1. 关闭任务队列
        m_taskQueue.Shutdown();
        // 2. 等待所有线程结束
        for (auto& thread_ptr : m_threads)
        {
            thread_ptr->join();
        }
    }
}

void ThreadPool::start()
{
    m_started = true;
    //找人进来
    m_threads.reserve(m_threadNum);
    //分配工作
    for (int i = 0; i < m_threadNum; ++i)
    {
        // 将 std::bind 替换为了下面的 lambda 表达式
        //lambda表达式其实就是对要完成函数的一个包装
        m_threads.emplace_back(std::make_unique<Thread>(
            [this]() { threadFunc(); }, 
            m_name + std::to_string(i)
        ));//创建过程并不执行threadFunc()，只是创建了一个线程对象
        m_threads[i]->start();
        //这里函数的执行切换了线程，所以这里的start()函数是异步的,while(true)不影响for函数的执行
    }
}

void ThreadPool::addTask(Task task)
{
    m_taskQueue.Push(std::move(task));
}

// 每个线程调用的不是一个简单的任务,而是这么一个流程,并且会因为while(true)一直工作
void ThreadPool::threadFunc()
{
    LOG_INFO << "Worker thread started in thread " << std::this_thread::get_id();
    while (true)
    {
        // Pop 现在返回一个 optional
        auto task_opt = m_taskQueue.Pop();

        if (task_opt)
        {
            // 如果 optional 有值，就执行任务
            Task task = task_opt.value();
            if (task)
            {
                task();
            }
        }
        else // 如果返回的是空 optional，说明队列已关闭且为空
        {
            // 收到“下班”信号，退出循环
            break; 
        }
    }
    LOG_INFO << "Worker thread exited.";
}

void ThreadPool::shutdown()
{
    // 加一个判断，防止重复关闭
    if (!m_started) {
        return;
    }

    LOG_INFO << "ThreadPool is shutting down...";

    // 1. 关闭任务队列的入口，这是调用内部成员 LockQueue 的 Shutdown
    m_taskQueue.Shutdown(); 
    
    // 2. 等待并回收所有工作线程
    for (auto& thread_ptr : m_threads)
    {
        thread_ptr->join();
    }

    // 3. 清理状态
    m_threads.clear();
    m_started = false;
}