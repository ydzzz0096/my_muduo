#ifndef CHANNEL_H
#define CHANNEL_H

#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include <functional>
#include <memory>

class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // 事件的分发,被Eventloop调用
    // handleEvent 的唯一职责，就是检查具体发生了哪些事件 (m_revents)，然后调用相应的回调函数
    void handleEvent(Timestamp receiveTime);

    // 设置者: 通常是封装了 Channel 的那个类，比如 TcpConnection 或 Acceptor
    void setReadCallback(const ReadEventCallback& cb) { m_readCallback = cb; }
    void setWriteCallback(const EventCallback& cb) { m_writeCallback = cb; }
    void setCloseCallback(const EventCallback& cb) { m_closeCallback = cb; }
    void setErrorCallback(const EventCallback& cb) { m_errorCallback = cb; }

    //将 Channel 绑定到一个对象上(通过weak_ptr)，防止在对象被销毁后,还调用相应的回调函数,访问不存在的内存
    // TcpConnection 在创建并设置好 Channel 后（通常在 connectEstablished 中），会调用 channel->tie(shared_from_this())
    void tie(const std::shared_ptr<void>&);

    int fd() const { return m_fd; }
    int events() const { return m_events; }
    void set_revents(int revt) { m_revents = revt; }

    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();

    // 查询“兴趣状态”
    bool isNoneEvent() const { return m_events == kNoneEvent; }
    bool isWriting() const { return m_events & kWriteEvent; }
    bool isReading() const { return m_events & kReadEvent; }

    int index() const { return m_index; }
    void set_index(int idx) { m_index = idx; }

    EventLoop* ownerLoop() { return m_loop; }
    void remove();

private:
    void update();
    
    // handleEventWithGuard 这个函数才是真正“干活”的地方
    // handleEvent检查过所指对象的状态后,在内部调用
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* m_loop;// 为了调用update向上汇报工作
    const int m_fd;// 关注的连接
    int m_events;// 关注的事件
    int m_revents;// 实际发生的事件
    int m_index; // used by Poller,记录channel在map中的状态,new/added/delete

    /*为了避免循环引用。TcpConnection 通常拥有 Channel（比如通过 unique_ptr），如果 Channel 再用 shared_ptr 强引用 TcpConnection，两者就会互相“抓住”对方，导致谁也无法被正常析构，造成内存泄漏。weak_ptr 只“观察”对象，不增加引用计数，完美地解决了这个问题*/
    std::weak_ptr<void> m_tie;
    bool m_tied;

    ReadEventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_closeCallback;
    EventCallback m_errorCallback;
};
#endif