// net/TcpConnection.h

#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include "InetAddress.h"
#include "Callbacks.h" // 稍后我们会创建这个新文件
#include "Buffer.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;
//为什么这么复杂 : Acceptor 的业务是**“一次性”的，而 TcpConnection 的业务是“全周期”,“有状态”**和存在异步部分的。
/**
 * TcpConnection 是服务器与客户端之间连接的抽象。
 * 它负责管理 socket fd，并通过 Channel 将其注册到 EventLoop。
 * 它拥有 inputBuffer 和 outputBuffer，用于处理读写事件。
 * 它是 noncopyable 的，因为每个 TcpConnection 对象都代表一个唯一的连接。
 * 生命周期需要关注,多方决定,可能被一个线程创建而被另一个线程销毁
 */
// 安全的获取一个指向自己的 shared_ptr 
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return m_loop; }
    const std::string& name() const { return m_name; }
    const InetAddress& localAddress() const { return m_localAddr; }
    const InetAddress& peerAddress() const { return m_peerAddr; }

    bool connected() const { return m_state == kConnected; }

    // 发送数据
    void send(const std::string& buf);
    // 关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb) { m_connectionCallback = cb; }
    void setMessageCallback(const MessageCallback& cb) { m_messageCallback = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { m_writeCompleteCallback = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) 
    { m_highWaterMarkCallback = cb; m_highWaterMark = highWaterMark; }
    void setCloseCallback(const CloseCallback& cb) { m_closeCallback = cb; }

    // 当 TcpServer 接受一个新连接时调用
    void connectEstablished();
    // 当 TcpServer 移除一个连接时调用
    void connectDestroyed();

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE state) { m_state = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();

    EventLoop* m_loop;  
    const std::string m_name;
    std::atomic<StateE> m_state;
    bool m_reading;

    // 这里和 Acceptor 类似   Acceptor => mainLoop    TcpConnection => subLoop
    std::unique_ptr<Socket> m_socket;
    std::unique_ptr<Channel> m_channel;

    // 身份认证,记载建立连接的双方
    const InetAddress m_localAddr;
    const InetAddress m_peerAddr;

    ConnectionCallback m_connectionCallback; // 状态更新时的回调,是为了让应用层有机会更新(添加或删除连接)
    MessageCallback m_messageCallback;       // 有读写消息时的回调
    WriteCompleteCallback m_writeCompleteCallback; // 消息发送完成以后的回调
    HighWaterMarkCallback m_highWaterMarkCallback;
    CloseCallback m_closeCallback;
    size_t m_highWaterMark;

    Buffer m_inputBuffer;  // 接收数据的缓冲区
    Buffer m_outputBuffer; // 发送数据的缓冲区
};

#endif