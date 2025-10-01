// net/EventLoop.cpp
#include "EventLoop.h"
#include "base/Logger.h"

// TLS: Thread-Local Storage
// ?
__thread EventLoop* t_loopInThisThread = nullptr;

EventLoop::EventLoop()
    : m_looping(false),
      m_quit(false),
      m_threadId(CurrentThread::tid())
{
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << m_threadId;
    }
    else
    {
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop()
{
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    m_looping = true;
    m_quit = false;
    LOG_INFO << "EventLoop " << this << " start looping";

    while (!m_quit)
    {
        // 这里暂时是空的，之后会填充 Poller->poll()
        LOG_INFO << "EventLoop " << this << " is looping...";
        ::sleep(1); // 睡眠1秒，模拟阻塞等待
    }

    LOG_INFO << "EventLoop " << this << " stop looping.";
    m_looping = false;
}

void EventLoop::quit()
{
    m_quit = true;
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId = " << m_threadId
              << ", current thread id = " << CurrentThread::tid();
}