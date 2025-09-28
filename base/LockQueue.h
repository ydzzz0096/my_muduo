// base/LockQueue.h

#ifndef LOCKQUEUE_H
#define LOCKQUEUE_H

#include <queue>
#include <mutex>              // 引入互斥锁
#include <condition_variable> // 引入条件变量

// 模板化的线程安全阻塞队列
template<typename T>
class LockQueue
{
public:
    // 生产者：将任务放入队列
    void Push(const T& data)
    {
        // std::lock_guard 实现了 RAII，在构造时自动上锁，析构时自动解锁
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        // 唤醒一个可能正在等待的消费者线程
        m_condvariable.notify_one();
    }

    // 消费者：从队列中取出任务
    T Pop()
    {
        // std::unique_lock 更灵活，是配合条件变量使用的标准选择
        std::unique_lock<std::mutex> lock(m_mutex);

        // 使用 while 循环来防止“虚假唤醒”(spurious wakeup)
        while (m_queue.empty())
        {
            // 当队列为空时:
            // 1. 自动释放 lock
            // 2. 将当前线程置于等待（阻塞）状态
            // 3. 当被唤醒时，它会重新尝试获取锁，然后继续
            m_condvariable.wait(lock);
        }

        // 取出队首元素
        T data = m_queue.front();
        m_queue.pop();
        return data;
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvariable;
};

#endif