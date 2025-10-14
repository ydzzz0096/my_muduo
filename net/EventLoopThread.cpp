// net/EventLoopThread.cpp (新增文件)

#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : m_loop(nullptr),
      m_exiting(false),
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
    m_thread.start(); // 启动底层的新线程

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_loop == nullptr)
        {
            m_cond.wait(lock);
        }
        loop = m_loop;
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的 EventLoop，和上面的线程一一对应
    
    if (m_callback)
    {
        m_callback(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_loop = &loop;
        m_cond.notify_one();
    }

    loop.loop(); // EventLoop::loop() => Poller::poll()
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loop = nullptr;
}