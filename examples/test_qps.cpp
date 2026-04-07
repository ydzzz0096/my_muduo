#include "base/Logger.h"
#include "base/Timestamp.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/TcpClient.h"

#include <atomic>
#include <memory>
#include <signal.h>
#include <thread>
#include <unistd.h>
#include <vector>

// === 全局统计量 (原子操作) ===
std::atomic<int64_t> g_totalMessages(0);
std::atomic<int64_t> g_totalBytes(0);
std::atomic<int64_t> g_totalLatency(0); // 新增：总往返时延（微秒）

// 消息内容 (固定 36 字节)
std::string g_message = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// === 压测客户端类 ===
class QpsClient {
public:
  QpsClient(EventLoop *loop, const InetAddress &serverAddr,
            const std::string &name)
      : client_(loop, serverAddr, name), lastSendTime_(0) {
    client_.setConnectionCallback(
        std::bind(&QpsClient::onConnection, this, std::placeholders::_1));
    client_.setMessageCallback(
        std::bind(&QpsClient::onMessage, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));
  }

  void connect() { client_.connect(); }

private:
  void onConnection(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
      // 记录发送瞬间的时间戳
      lastSendTime_ = Timestamp::now().microSecondsSinceEpoch();
      conn->send(g_message);
    }
  }

  void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) {
    // 1. 计算当前这一个往返的时延
    int64_t now = Timestamp::now().microSecondsSinceEpoch();
    if (lastSendTime_ > 0) {
      g_totalLatency += (now - lastSendTime_);
    }

    // 2. 更新统计数据
    g_totalMessages++;
    g_totalBytes += buf->readableBytes();

    // 3. 准备下一次发送，更新时间戳实现 Ping-Pong 闭环
    lastSendTime_ = Timestamp::now().microSecondsSinceEpoch();
    conn->send(buf->retrieveAllAsString());
  }

  TcpClient client_;
  int64_t lastSendTime_; // 每个连接记录自己的上次发送时间
};

// === 统计线程 ===
void statsFunc() {
  int64_t oldMessages = 0;
  int64_t oldBytes = 0;
  int64_t oldLatency = 0;

  while (true) {
    sleep(1);

    int64_t newMessages = g_totalMessages;
    int64_t newBytes = g_totalBytes;
    int64_t newLatency = g_totalLatency;

    int64_t deltaMessages = newMessages - oldMessages;
    int64_t deltaBytes = newBytes - oldBytes;
    int64_t deltaLatency = newLatency - oldLatency;

    double throughput = static_cast<double>(deltaBytes) / (1024 * 1024);

    // 计算平均时延 (单位：毫秒 ms)
    double avgLatencyMs = 0.0;
    if (deltaMessages > 0) {
      avgLatencyMs = static_cast<double>(deltaLatency) / deltaMessages / 1000.0;
    }

    printf("Total QPS: %8ld, Throughput: %8.3f MiB/s, Avg Latency: %6.3f ms\n",
           deltaMessages, throughput, avgLatencyMs);

    oldMessages = newMessages;
    oldBytes = newBytes;
    oldLatency = newLatency;
  }
}

// === 工作线程与 Main 函数保持不变 ===
void threadFunc(const InetAddress &serverAddr, int connectionsPerThread,
                int threadIdx) {
  EventLoop loop;
  std::vector<std::unique_ptr<QpsClient>> clients;
  clients.reserve(connectionsPerThread);

  for (int i = 0; i < connectionsPerThread; ++i) {
    char buf[32];
    snprintf(buf, sizeof buf, "Client-T%d-%d", threadIdx, i);
    clients.emplace_back(new QpsClient(&loop, serverAddr, buf));
  }

  for (auto &client : clients) {
    client->connect();
  }

  loop.loop();
}

int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);
  Logger::getInstance().setLogLevel(ERROR);

  if (argc != 5) {
    printf("Usage: %s <ip> <port> <threads> <connections>\n", argv[0]);
    return 1;
  }

  std::string ip = argv[1];
  uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
  int threadCount = atoi(argv[3]);
  int totalConnections = atoi(argv[4]);
  int connectionsPerThread = totalConnections / threadCount;

  printf("Starting Latency & QPS Benchmark...\n");
  InetAddress serverAddr(port, ip);

  std::thread stats(statsFunc);
  stats.detach();

  std::vector<std::thread> threads;
  for (int i = 0; i < threadCount; ++i) {
    threads.emplace_back(threadFunc, serverAddr, connectionsPerThread, i);
  }

  for (auto &t : threads) {
    t.join();
  }

  return 0;
}