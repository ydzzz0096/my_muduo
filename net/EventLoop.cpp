#include "EventLoop.h"
#include "base/Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <memory>

__thread EventLoop* t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL << "eventfd error:" << errno;
    }
    return evtfd;
}

EventLoop::EventLoop()
    : m_looping(false),
      m_quit(false),
      m_callingPendingFunctors(false),
      m_threadId(CurrentThread::tid()),
      m_poller(Poller::newDefaultPoller(this)),
      m_wakeupFd(createEventfd()),
      m_wakeupChannel(std::make_unique<Channel>(this, m_wakeupFd))
{
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop exists in this thread";
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 固定不变的绑定关系,尽早确定下来
    // 唤醒成功之后应该调用handleRead清零计数器
    m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    m_wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
    m_wakeupChannel->disableAll();
    m_wakeupChannel->remove();
    ::close(m_wakeupFd);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    assertInLoopThread();
    m_looping = true;
    m_quit = false;
    LOG_INFO << "EventLoop " << this << " start looping";

    while (!m_quit)
    {
        m_activeChannels.clear();
        m_poller->poll(kPollTimeMs, &m_activeChannels);

        for (Channel* channel : m_activeChannels)
        {
            channel->handleEvent(Timestamp::now());
        }
        
        doPendingFunctors();
    }
    LOG_INFO << "EventLoop " << this << " stop looping.";
    m_looping = false;
}

void EventLoop::quit()
{
    m_quit = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(std::function<void()> cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(std::function<void()> cb)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingFunctors.push_back(cb);
    }

    if (!isInLoopThread() || m_callingPendingFunctors)
    {
        wakeup();
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(m_wakeupFd, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(m_wakeupFd, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<std::function<void()>> functors;
    m_callingPendingFunctors = true;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        functors.swap(m_pendingFunctors);
    }

    for (const auto& functor : functors)
    {
        functor();
    }
    m_callingPendingFunctors = false;
}

void EventLoop::updateChannel(Channel* channel)
{
    assertInLoopThread();
    m_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assertInLoopThread();
    m_poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assertInLoopThread();
    return m_poller->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId = " << m_threadId
              << ", current thread id = " << CurrentThread::tid();
}