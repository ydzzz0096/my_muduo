// net/EventLoopThreadPool.h

#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include "base/noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread; // 稍后创建这个辅助类

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { m_numThreads = numThreads; }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());// ThreadInitCallback由tcpserver设定

    // 如果工作在多线程中，baseLoop_ 默认以轮询的方式分配 channel 给 subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();
    bool started() const { return m_started; }
    const std::string& name() const { return m_name; }

private:
    EventLoop* m_baseLoop; // 用户创建的 EventLoop，即 mainLoop
    std::string m_name;
    bool m_started;
    int m_numThreads;
    size_t m_next;
    std::vector<std::unique_ptr<EventLoopThread>> m_threads;// EventLoopThread拥有thread
    std::vector<EventLoop*> m_loops;// 被子线程拥有所以需要单独一个表用来访问
};
#endif