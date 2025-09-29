// base/Thread.cpp

#include "Thread.h"
#include <sys/syscall.h>
#include <unistd.h>

std::atomic<int> Thread::m_numCreated(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : m_started(false),
      m_joined(false),
      m_tid(0),
      m_func(std::move(func)),
      m_name(name)
{
    setDefaultName();
    m_numCreated++;
}

Thread::~Thread()
{
    // 如果线程已经启动且没有被 join，则 detach
    // 确保 thread 对象析构时，程序不会因为 dangling thread 而 terminate
    if (m_started && !m_joined)
    {
        m_thread->detach();
    }
}

void Thread::start()
{
    m_started = true;
    // 使用 a lambda to capture `this` pointer
    m_thread = std::make_shared<std::thread>([this]() {
        // 获取线程的 tid
        m_tid = syscall(SYS_gettid);
        // 执行用户传入的线程函数
        m_func();
    });
}

void Thread::join()
{
    m_joined = true;
    if (m_thread && m_thread->joinable())
    {
        m_thread->join();
    }
}

void Thread::setDefaultName()
{
    int num = m_numCreated;
    if (m_name.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        m_name = buf;
    }
}