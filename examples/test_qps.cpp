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
#include <unistd.h>
#include <signal.h>

// 全局统计量
std::atomic<int64_t> g_totalMessages(0);
std::atomic<int64_t> g_totalBytes(0);

// 消息内容（固定长度）
std::string g_message = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // 36 bytes

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
        // 压测时不需要重连，断开就断开了
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
            // 连接成功，发送第一条消息，启动 Ping-Pong
            conn->send(g_message);
        }
        else
        {
            LOG_ERROR << "Client Disconnected";
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        // 统计数据
        g_totalMessages++;
        g_totalBytes += buf->readableBytes();
        
        // 取出数据并立刻发回去 (Ping-Pong)
        conn->send(buf->retrieveAllAsString());
    }

    TcpClient client_;
};

void statsFunc()
{
    int64_t oldMessages = 0;
    int64_t oldBytes = 0;
    int64_t startTime = Timestamp::now().microSecondsSinceEpoch();

    while (true)
    {
        sleep(1); // 每秒统计一次
        
        int64_t newMessages = g_totalMessages;
        int64_t newBytes = g_totalBytes;
        int64_t now = Timestamp::now().microSecondsSinceEpoch();
        
        int64_t deltaMessages = newMessages - oldMessages;
        int64_t deltaBytes = newBytes - oldBytes;
        double deltaSeconds = static_cast<double>(now - startTime) / 1000000.0;

        // 打印 QPS 和 吞吐量 (MiB/s)
        // 注意：这里我们直接用 printf/cout，因为日志系统可能被关了
        printf("QPS: %8ld, Throughput: %8.3f MiB/s\n", 
               deltaMessages, 
               static_cast<double>(deltaBytes) / (1024*1024));

        oldMessages = newMessages;
        oldBytes = newBytes;
        startTime = now;
    }
}

int main(int argc, char* argv[])
{
    // 【重要】忽略 SIGPIPE，防止服务器断开导致压测客户端崩溃
    signal(SIGPIPE, SIG_IGN);

    // 【重要】关闭无关日志，避免影响性能结果
    Logger::getInstance().setLogLevel(ERROR);

    if (argc != 4)
    {
        printf("Usage: %s <ip> <port> <connections>\n", argv[0]);
        return 1;
    }

    std::string ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    int connectionNum = atoi(argv[3]);

    printf("Starting Benchmark: Server=%s:%d, Connections=%d, MsgSize=%lu\n", 
           ip.c_str(), port, connectionNum, g_message.size());

    EventLoop loop;
    InetAddress serverAddr(port, ip);
    
    // 创建指定数量的客户端
    std::vector<std::unique_ptr<QpsClient>> clients;
    for (int i = 0; i < connectionNum; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Client-%d", i);
        clients.emplace_back(new QpsClient(&loop, serverAddr, buf));
    }

    // 启动所有客户端
    for (auto& client : clients)
    {
        client->connect();
    }

    // 启动统计线程
    std::thread statsThread(statsFunc);
    statsThread.detach();

    // 启动事件循环
    loop.loop();

    return 0;
}