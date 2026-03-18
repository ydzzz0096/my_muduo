#include "Buffer.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "base/Timestamp.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

class EchoServer {
public:
  EchoServer(EventLoop *loop, const InetAddress &listenAddr)
      : server_(loop, listenAddr, "EchoServer") {
    server_.setConnectionCallback([this](const TcpConnectionPtr &conn) {
      if (conn->connected()) {
        connectionCount_++;
      } else {
        connectionCount_--;
      }
    });
    server_.setMessageCallback(
        [this](const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) {
          string msg = buf->retrieveAllAsString();
          messageCount_++;
          byteCount_ += msg.size();
          conn->send(msg);
        });
    server_.setThreadNum(8);
  }

  void start() { server_.start(); }

  atomic<int64_t> connectionCount_{0};
  atomic<int64_t> messageCount_{0};
  atomic<int64_t> byteCount_{0};

private:
  TcpServer server_;
};

int main() {
  cout << "Starting MyMuduo Benchmark Server..." << endl;

  EventLoop loop;
  InetAddress listenAddr(8000);
  EchoServer server(&loop, listenAddr);
  server.start();

  cout << "Server started on port 8000" << endl;
  cout << "Use test_qps client to benchmark" << endl;

  // 定期打印统计信息
  thread statsThread([&server]() {
    while (true) {
      this_thread::sleep_for(chrono::seconds(5));
      cout << "Connections: " << server.connectionCount_.load()
           << ", Messages: " << server.messageCount_.load()
           << ", Bytes: " << server.byteCount_.load() << endl;
    }
  });
  statsThread.detach();

  loop.loop();
  return 0;
}
