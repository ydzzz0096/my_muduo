// base/LockQueue.h

#ifndef LOCKQUEUE_H
#define LOCKQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional> // 1. 引入 C++17 的 std::optional

template<typename T>
class LockQueue
{
public:
    LockQueue() : m_shutdown(false) {}

    void Push(const T& data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_shutdown) return; // 关闭后不再接受新任务
        m_queue.push(data);
        m_condvariable.notify_one();
    }

    // 2. 修改 Pop 返回类型为 std::optional<T>
    std::optional<T> Pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        m_condvariable.wait(lock, [this] {
            return !m_queue.empty() || m_shutdown;
        });

        // 3. 如果队列已关闭且为空，返回一个“空”的 optional
        if (m_shutdown && m_queue.empty()) {
            return std::nullopt;
        }

        T data = m_queue.front();
        m_queue.pop();
        return data;
    }

    // 4. 新增 Shutdown 方法
    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdown = true;
        m_condvariable.notify_all(); // 唤醒所有线程
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvariable;
    bool m_shutdown; // 关闭标志
};

#endif