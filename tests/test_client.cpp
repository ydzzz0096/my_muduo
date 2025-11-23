// examples/test_client.cpp

#include "net/TcpClient.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "base/Logger.h"
#include "net/Buffer.h"

#include <iostream>
#include <string>

class EchoClient
{
public:
    EchoClient(EventLoop* loop, const InetAddress& listenAddr, const std::string& name)
        : m_client(loop, listenAddr, name)
    {
        m_client.setConnectionCallback(
            std::bind(&EchoClient::onConnection, this, std::placeholders::_1));
        
        m_client.setMessageCallback(
            std::bind(&EchoClient::onMessage, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        m_client.enableRetry();
    }

    void connect()
    {
        m_client.connect();
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "Connected to server!";
            // 连接成功后，立刻发送一条消息
            conn->send("Hello, Muduo!");
        }
        else
        {
            LOG_INFO << "Disconnected from server.";
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO << "Client received: " << msg;
        // 这里可以做个简单的测试：收到什么就稍微改一下发回去，或者结束
        // sleep(1);
        // conn->send("Ping");
    }

    TcpClient m_client;
};

int main()
{
    EventLoop loop;
    InetAddress serverAddr(8000, "127.0.0.1"); // 连接我们之前的 EchoServer
    
    EchoClient client(&loop, serverAddr, "EchoClient");
    client.connect();
    
    loop.loop(); // 启动事件循环
    return 0;
}