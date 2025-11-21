// net/EventLoopThreadPool.cpp

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg)
    : m_baseLoop(baseLoop),
      m_name(nameArg),
      m_started(false),
      m_numThreads(0),
      m_next(0)
{}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    m_started = true;
    for (int i = 0; i < m_numThreads; ++i)
    {
        char buf[m_name.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", m_name.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        m_threads.push_back(std::unique_ptr<EventLoopThread>(t));
        m_loops.push_back(t->startLoop()); // 底层创建线程，并启动 EventLoop
    }

    // 单线程情况
    if (m_numThreads == 0 && cb)
    {
        cb(m_baseLoop);
    }
}

// 如果工作在多线程中，baseLoop_ 默认以轮询的方式分配 channel 给 subloop
EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop* loop = m_baseLoop;// 初始化,应对单线程情况
    if (!m_loops.empty()) // 通过轮询获取下一个处理事件的 loop
    {
        loop = m_loops[m_next];
        ++m_next;
        if (m_next >= m_loops.size())
        {
            m_next = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if (m_loops.empty())
    {
        return std::vector<EventLoop*>(1, m_baseLoop);
    }
    else
    {
        return m_loops;
    }
}