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

    void handleEvent(Timestamp receiveTime);

    void setReadCallback(const ReadEventCallback& cb) { m_readCallback = cb; }
    void setWriteCallback(const EventCallback& cb) { m_writeCallback = cb; }
    void setCloseCallback(const EventCallback& cb) { m_closeCallback = cb; }
    void setErrorCallback(const EventCallback& cb) { m_errorCallback = cb; }

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
    
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* m_loop;// 为了调用update向上汇报工作
    const int m_fd;
    int m_events;// 关注的事件
    int m_revents;// 实际发生的事件
    int m_index; // used by Poller

    ReadEventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_closeCallback;
    EventCallback m_errorCallback;
};
#endif