// net/Connector.h

#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "base/noncopyable.h"
#include "InetAddress.h"
#include "Channel.h"

#include <functional>
#include <memory>
#include <atomic>

class EventLoop;

/**
 * @brief 负责主动发起 TCP 连接的类。
 * 核心逻辑：
 * 1. 调用 ::connect。
 * 2. 如果返回 EINPROGRESS，则注册 Channel 监听 EPOLLOUT (可写) 事件。
 * 3. 当 Channel 可写时，通过 getsockopt(SO_ERROR) 检查连接是否真的成功。
 * 4. 如果成功，回调 NewConnectionCallback；如果失败，执行重连逻辑。
 */
class Connector : noncopyable, public std::enable_shared_from_this<Connector>
{
public:
    using NewConnectionCallback = std::function<void(int sockfd)>;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        m_newConnectionCallback = cb;
    }

    void start();   // 开始连接
    void restart(); // 重启连接
    void stop();    // 停止连接

    const InetAddress& serverAddress() const { return m_serverAddr; }

private:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30 * 1000; // 最大重连间隔 30s
    static const int kInitRetryDelayMs = 500;      // 初始重连间隔 0.5s

    void setState(States s) { m_state = s; }
    void startInLoop();
    void stopInLoop();
    
    void connect();     // 执行 ::connect 系统调用
    void connecting(int sockfd); // 处理 EINPROGRESS 状态
    
    void handleWrite(); // Channel 的回调：连接建立完成（或失败）
    void handleError(); 
    void resetChannel();
    
    void retry(int sockfd); // 重连逻辑
    int removeAndResetChannel(); // 移除 Channel 并重置指针

    EventLoop* m_loop;
    InetAddress m_serverAddr;
    std::atomic_bool m_connect; // 是否允许连接的标志
    std::atomic<States> m_state; 
    
    std::unique_ptr<Channel> m_channel; // 负责监听 connecting 状态下的 socket
    NewConnectionCallback m_newConnectionCallback;
    
    int m_retryDelayMs; // 当前重连间隔
};

#endif