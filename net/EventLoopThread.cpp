// net/EventLoopThread.cpp (新增文件)

#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : m_loop(nullptr),
      m_exiting(false),
      /*
        成员函数不能像普通函数那样被直接调用，它必须由一个类的对象实例来调用（比如 myLoopThread->threadFunc()）。
        
        std::bind 的作用就是将“动作”（&EventLoopThread::threadFunc）和“执行者”（this）**“捆绑”**在一起
    */
      m_thread(std::bind(&EventLoopThread::threadFunc, this), name),
      m_mutex(),
      m_cond(),
      m_callback(cb)
{}

EventLoopThread::~EventLoopThread()
{
    m_exiting = true;
    if (m_loop != nullptr)
    {
        m_loop->quit();
        m_thread.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    m_thread.start(); // 1. 通知子线程“开始工作！”

    EventLoop* loop = nullptr;
    {
        // 2. 锁住 m_mutex。
        //    必须用 unique_lock，因为它需要配合 condition_variable
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // 3. 检查子线程是否已“准备就绪”
        while (m_loop == nullptr)
        {
            // 4.【核心】
            //    如果没就绪，就调用 m_cond.wait()：
            //    a. 原子地“解锁” m_mutex (允许子线程进入临界区)
            //    b. 让父线程进入“沉睡”状态
            //    c. (当被子线程唤醒时) 重新“加锁” m_mutex 并再次检查 while 条件
            m_cond.wait(lock);
        }
        
        // 5. (子线程已唤醒我)
        //    此时，父线程 100% 确定 m_loop 已经被子线程安全地赋值了
        loop = m_loop;
    }
    return loop; // 6. 安全地返回有效的 EventLoop 指针
}

void EventLoopThread::threadFunc()
{
    EventLoop loop; // 1. 子线程创建自己的 EventLoop (在栈上，不共享)
    
    // 这里调用了ThreadInitCallback& cb.被打包了很合理
    if (m_callback)
    {
        m_callback(&loop);
    }

    {
        // 2. 锁住 m_mutex。
        //    (此时父线程要么在 wait() 中沉睡并已释放锁，
        //     要么还没来得及加锁，无论如何，子线程都能安全地拿到锁)
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // 3.【核心】
        //    在锁的保护下，安全地修改共享变量 m_loop
        m_loop = &loop;
        
        // 4.【核心】
        //    通知那个正在 wait() 中沉睡的父线程：“我准备好了，你可以醒来了！”
        m_cond.notify_one();
    } // 5. 锁在这里被释放

    loop.loop(); // 6. 子线程开始它自己的工作循环
    
    // 7. (loop.loop() 退出后，线程即将结束)
    //    再次加锁，安全地清空 m_loop 指针
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loop = nullptr;
}