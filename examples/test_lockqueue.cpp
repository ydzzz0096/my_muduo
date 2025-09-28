// examples/test_lockqueue.cpp

#include "base/LockQueue.h" // 引入我们实现的头文件
#include <iostream>
#include <thread>
#include <string>
#include <chrono> // for std::chrono::milliseconds

// 创建一个全局的队列实例
LockQueue<int> g_queue;
// 创建一个全局的互斥锁，专门用来保护 cout
std::mutex g_cout_mutex;

void producer()
{
    for (int i = 0; i < 10; ++i)
    {
        g_queue.Push(i);
        {
            std::lock_guard<std::mutex> lock(g_cout_mutex);
            std::cout << "Producer pushed: " << i << std::endl;
        }
        // 稍作延时，让消费者有时间追赶，更好地观察交错执行
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
}

void consumer()
{
    for (int i = 0; i < 10; ++i)
    {
        int data = g_queue.Pop();
        {
            std::lock_guard<std::mutex> lock(g_cout_mutex);
            std::cout << "                Consumer popped: " << data << std::endl;
        }
    }
}

int main()
{
    std::thread t1(producer);
    std::thread t2(consumer);

    t1.join();
    t2.join();
    
    return 0;
}