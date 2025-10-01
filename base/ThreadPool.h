// base/ThreadPool.h

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "noncopyable.h"
#include "Thread.h"
#include "LockQueue.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class ThreadPool : noncopyable
{
public:
    using Task = std::function<void()>;

    ThreadPool(int threadNum, const std::string& name = std::string("ThreadPool"));
    ~ThreadPool();

    // 启动线程池
    void start();

    // 向线程池添加任务
    void addTask(Task task);

    void shutdown();

    
private:
    void threadFunc(); // 线程池中每个工作线程运行的函数

    std::string m_name;
    int m_threadNum;//线程池应该有多少个线程
    std::vector<std::unique_ptr<Thread>> m_threads; // 线程列表  这里用unique_ptr是因为线程池应该独享线程资源,管理其中线程的生命周期
    LockQueue<Task> m_taskQueue; // 任务队列
    std::atomic_bool m_started;// 必须假设它可能会在更复杂的环境中使用——比如，一个线程创建并 start() 线程池，而另一个线程在未来的某个时刻决定要销毁这个线程池
};

#endif