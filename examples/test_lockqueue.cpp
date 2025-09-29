// examples/test_lockqueue.cpp

#include "base/LockQueue.h"
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <optional> // 确保包含了 optional 头文件

// 全局队列实例 (保持不变)
LockQueue<int> g_queue;
// 全局互斥锁，保护 cout (保持不变)
std::mutex g_cout_mutex;

// 生产者函数 (保持不变)
void producer()
{
    for (int i = 0; i < 10; ++i)
    {
        g_queue.Push(i);
        {
            std::lock_guard<std::mutex> lock(g_cout_mutex);
            std::cout << "Producer pushed: " << i << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
}

// 【核心修正】更新消费者函数
void consumer()
{
    for (int i = 0; i < 10; ++i)
    {
        // 1. Pop() 现在返回一个 optional 对象
        std::optional<int> data_opt = g_queue.Pop();

        // 2. 检查 optional 是否真的包含一个值
        if (data_opt.has_value())
        {
            // 3. 如果有值，通过 .value() 方法取出它
            int data = data_opt.value();
            {
                std::lock_guard<std::mutex> lock(g_cout_mutex);
                std::cout << "                Consumer popped: " << data << std::endl;
            }
        }
        // 在这个简单的测试中，我们不处理队列关闭的情况，所以可以省略 else
    }
}

// main 函数 (保持不变)
int main()
{
    std::thread t1(producer);
    std::thread t2(consumer);

    t1.join();
    t2.join();
    
    return 0;
}