#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "base/Logger.h"
#include <string>
#include <functional>
#include "net/TcpConnection.h" // 【核心修正】添加 TcpConnection 头文件
#include "net/Buffer.h"        // 【核心修正】添加 Buffer 头文件


class EchoServer
{
public:
    EchoServer(EventLoop* loop,
            const InetAddress& addr,
            const std::string& name)
        : m_server(loop, addr, name),
          m_loop(loop)
    {
        m_server.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        m_server.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        
        m_server.setThreadNum(3);
    }

    void start()
    {
        m_server.start();
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "Connection UP : " << conn->peerAddress().toIpPort();
        }
        else
        {
            LOG_INFO << "Connection DOWN : " << conn->peerAddress().toIpPort();
        }
    }

    void onMessage(const TcpConnectionPtr& conn,
                Buffer* buf,
                Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        // conn->shutdown(); // 发送完数据就关闭连接
    }
    
    EventLoop* m_loop;
    TcpServer m_server;
};


int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01");
    server.start(); // listenfd epoll_ctl=>epoll
    loop.loop(); // 启动 mainloop 的底层 Poller
    
    return 0;
}