// examples/test_qps.cpp

#include "net/TcpClient.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "base/Logger.h"
#include "base/Timestamp.h"
#include "base/Thread.h"

#include <vector>
#include <atomic>
#include <iostream>
#include <thread>
#include <memory>
#include <unistd.h>
#include <signal.h>

// === 全局统计量 (原子操作，天然线程安全) ===
std::atomic<int64_t> g_totalMessages(0);
std::atomic<int64_t> g_totalBytes(0);

// 消息内容 (固定 36 字节)
std::string g_message = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// === 压测客户端类 ===
class QpsClient
{
public:
    QpsClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name)
        : client_(loop, serverAddr, name)
    {
        client_.setConnectionCallback(
            std::bind(&QpsClient::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&QpsClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // 压测追求极限性能，通常不开启重连，断了就断了
    }

    void connect()
    {
        client_.connect();
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            // 连接建立，立刻发送第一条消息
            conn->send(g_message);
        }
        else
        {
            // 压测结束或异常断开，不打印 Error 避免刷屏
            // LOG_ERROR << "Client Disconnected"; 
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        // 统计：原子加
        g_totalMessages++;
        g_totalBytes += buf->readableBytes();
        
        // Ping-Pong：收到什么发什么
        conn->send(buf->retrieveAllAsString());
    }

    TcpClient client_;
};

// === 统计线程 (每秒打印一次总数据) ===
void statsFunc()
{
    int64_t oldMessages = 0;
    int64_t oldBytes = 0;
    int64_t startTime = Timestamp::now().microSecondsSinceEpoch();

    while (true)
    {
        sleep(1); 
        
        int64_t newMessages = g_totalMessages;
        int64_t newBytes = g_totalBytes;
        int64_t now = Timestamp::now().microSecondsSinceEpoch();
        
        int64_t deltaMessages = newMessages - oldMessages;
        int64_t deltaBytes = newBytes - oldBytes;
        
        // 计算真正的 MiB/s
        double throughput = static_cast<double>(deltaBytes) / (1024 * 1024);

        printf("Total QPS: %8ld, Throughput: %8.3f MiB/s\n", 
               deltaMessages, throughput);

        oldMessages = newMessages;
        oldBytes = newBytes;
        startTime = now;
    }
}

// === 工作线程 (每个线程跑一个 EventLoop) ===
void threadFunc(const InetAddress& serverAddr, int connectionsPerThread, int threadIdx)
{
    EventLoop loop; // 每个线程独立的 Loop
    
    std::vector<std::unique_ptr<QpsClient>> clients;
    clients.reserve(connectionsPerThread);

    for (int i = 0; i < connectionsPerThread; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Client-T%d-%d", threadIdx, i);
        clients.emplace_back(new QpsClient(&loop, serverAddr, buf));
    }

    for (auto& client : clients)
    {
        client->connect();
    }

    loop.loop(); // 在此阻塞
}

int main(int argc, char* argv[])
{
    // 1. 忽略 SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    // 2. 全局静音 (只打印 ERROR)
    Logger::getInstance().setLogLevel(ERROR);

    if (argc != 5)
    {
        printf("Usage: %s <ip> <port> <threads> <connections>\n", argv[0]);
        printf("Example: %s 127.0.0.1 8000 8 1000\n", argv[0]);
        return 1;
    }

    std::string ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    int threadCount = atoi(argv[3]);       // 线程数
    int totalConnections = atoi(argv[4]);  // 总连接数

    // 计算每个线程分摊多少连接
    int connectionsPerThread = totalConnections / threadCount;
    
    printf("Starting Benchmark:\n");
    printf("  Server: %s:%d\n", ip.c_str(), port);
    printf("  Threads: %d\n", threadCount);
    printf("  Total Connections: %d (%d per thread)\n", totalConnections, connectionsPerThread);
    printf("  Message Size: %lu bytes\n", g_message.size());
    printf("------------------------------------------------\n");

    InetAddress serverAddr(port, ip);

    // 3. 启动统计线程
    std::thread stats(statsFunc);
    stats.detach();

    // 4. 启动工作线程
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back(threadFunc, serverAddr, connectionsPerThread, i);
    }

    // 5. 等待所有线程 (实际上压测不会主动停止，这里会一直阻塞)
    for (auto& t : threads)
    {
        t.join();
    }

    return 0;
}