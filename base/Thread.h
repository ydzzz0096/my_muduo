// base/Thread.h

#ifndef THREAD_H
#define THREAD_H

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h" // 稍后我们会创建这个辅助类

// 线程类，封装 std::thread
class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return m_started; }
    pid_t tid() const { return m_tid; }
    const std::string& name() const { return m_name; }

    static int numCreated() { return m_numCreated; }

private:
    void setDefaultName();

    bool m_started;
    bool m_joined;

    std::shared_ptr<std::thread> m_thread; // 使用智能指针管理 thread 对象
    pid_t m_tid;
    
    ThreadFunc m_func;
    std::string m_name;
    static std::atomic<int> m_numCreated; // 统计创建的线程数量用来计数
};

#endif