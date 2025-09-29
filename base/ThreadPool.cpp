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
    m_threads.reserve(m_threadNum);
    for (int i = 0; i < m_threadNum; ++i)
    {
        m_threads.emplace_back(std::make_unique<Thread>(
            std::bind(&ThreadPool::threadFunc, this),
            m_name + std::to_string(i)
        ));
        m_threads[i]->start();
    }
}

void ThreadPool::addTask(Task task)
{
    m_taskQueue.Push(std::move(task));
}

// 【核心】修改工作线程的循环逻辑
void ThreadPool::threadFunc()
{
    LOG_INFO << "Worker thread started in thread " << std::this_thread::get_id();
    while (true)
    {
        // Pop 现在返回一个 optional
        auto task_opt = m_taskQueue.Pop();

        if (task_opt.has_value())
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