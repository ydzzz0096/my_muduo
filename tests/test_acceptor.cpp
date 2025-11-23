// examples/test_acceptor.cpp

#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Socket.h"
#include "base/Logger.h"
#include <unistd.h>

// 这是一个测试用的“新连接回调函数”
void newConnectionCallback(int sockfd, const InetAddress& peerAddr)
{
    LOG_INFO << "New connection arrived! sockfd: " << sockfd 
             << ", peer address: " << peerAddr.toIpPort();
    // 接受连接后，为了测试，我们直接关闭它
    ::close(sockfd);
}

int main()
{
    LOG_INFO << "Main thread started.";
    
    EventLoop loop;
    InetAddress listenAddr(9981); // 监听 9981 端口

    Acceptor acceptor(&loop, listenAddr, true);
    // 将我们的测试回调，设置给 Acceptor
    acceptor.setNewConnectionCallback(newConnectionCallback);
    acceptor.listen();

    LOG_INFO << "Server is listening on port 9981...";
    loop.loop(); // 启动事件循环

    return 0;
}