// net/TimerQueue.cpp

#include "TimerQueue.h"
#include "Timer.h"
#include "TimerId.h"
#include "EventLoop.h"
#include "base/Logger.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cassert>

namespace
{

// 创建 timerfd
int createTimerfd()
{
    // CLOCK_MONOTONIC: 单调时钟，不受系统时间被管理员修改的影响 (比如从10点改成9点)
    // TFD_NONBLOCK: 非阻塞
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_FATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

// 计算还有多久过期，并转换成内核需要的 timespec 结构
struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                         - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100; // 至少等待 100 微秒
    }
    
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

// 处理 timerfd 的读事件 (清除就绪状态)
void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    if (n != sizeof howmany)
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

// 重置 timerfd 的过期时间
void resetTimerfd(int timerfd, Timestamp expiration)
{
    // 唤醒 EventLoop 的时间点
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);

    // 核心：计算相对时间，设置超时
    newValue.it_value = howMuchTimeFromNow(expiration);
    
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_ERROR << "timerfd_settime()";
    }
}

} // namespace

TimerQueue::TimerQueue(EventLoop* loop)
    : m_loop(loop),
      m_timerfd(createTimerfd()),
      m_timerfdChannel(loop, m_timerfd),
      m_timers(),
      m_callingExpiredTimers(false)
{
    // 设置回调：当 timerfd 有事件（超时）时，执行 handleRead
    m_timerfdChannel.setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    // 开启监听
    m_timerfdChannel.enableReading();
}

TimerQueue::~TimerQueue()
{
    m_timerfdChannel.disableAll();
    m_timerfdChannel.remove();
    ::close(m_timerfd);
    // 清理所有 Timer
    for (const auto& timer : m_timers)
    {
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(std::function<void()> cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    m_loop->runInLoop(
        std::bind(&TimerQueue::insert, this, timer));
        
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    // 必须保证线程安全，将操作派发到 IO 线程
    m_loop->runInLoop(
        std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

// 【新增】需要在 TimerQueue.h 的 private 中声明 void cancelInLoop(TimerId timerId);
void TimerQueue::cancelInLoop(TimerId timerId)
{
    m_loop->assertInLoopThread();
    assert(m_timers.size() == m_activeTimers.size());

    // 构造一个 ActiveTimer 对象用于查找
    ActiveTimer timer(timerId.m_timer, timerId.m_sequence);

    // 查找该定时器是否存在于活跃列表中
    auto it = m_activeTimers.find(timer);
    if (it != m_activeTimers.end())
    {
        // 情况1：定时器还在队列中，直接删除
        size_t n = m_timers.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first; // FIXME: no delete please (如果是用 unique_ptr 管理则不需要)
        m_activeTimers.erase(it);
    }
    else if (m_callingExpiredTimers)
    {
        // 情况2：定时器正在执行回调（自我取消），此时它不在 m_activeTimers 中
        // 我们把它加入“待取消列表”，防止 reset 时被重启
        m_cancelingTimers.insert(timer);
    }
    assert(m_timers.size() == m_activeTimers.size());
}

// 核心逻辑：向 set 中插入定时器
bool TimerQueue::insert(Timer* timer)
{
    m_loop->assertInLoopThread(); // 必须在 IO 线程
    
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    auto it = m_timers.begin();
    
    // 如果 timers 为空，或者新定时器的过期时间比当前最早的还要早
    if (it == m_timers.end() || when < it->first)
    {
        earliestChanged = true;
    }
    
    // 插入到 set 中
    m_timers.insert(Entry(when, timer));
    m_activeTimers.insert(ActiveTimer(timer, timer->sequence()));

    // 如果最早过期时间变了，需要重置 timerfd
    if (earliestChanged)
    {
        resetTimerfd(m_timerfd, when);
    }
    return earliestChanged;
}

// timerfd 可读（超时）时的回调
void TimerQueue::handleRead()
{
    m_loop->assertInLoopThread();
    Timestamp now(Timestamp::now());
    
    // 1. 清除该事件，避免一直触发
    readTimerfd(m_timerfd, now);

    // 2. 获取所有已超时的定时器
    std::vector<Timer*> expired = getExpired(now);

    m_callingExpiredTimers = true;
    m_cancelingTimers.clear();

    // 3. 执行回调
    for (const auto& timer : expired)
    {
        timer->run();
    }
    m_callingExpiredTimers = false;

    // 4. 重置（如果是重复定时器，重新加入集合；否则销毁）
    reset(expired, now);
}

std::vector<Timer*> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Timer*> expired;
    
    // 创建一个哨兵值 (now, UINTPTR_MAX)
    // 它的意思是：同一时刻的 Timer，地址最大的那个（也就是这一时刻的所有 Timer）
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    
    // lower_bound 返回第一个 >= sentry 的迭代器
    // 所以 [begin, end) 就是所有 <= now 的定时器，即已过期的
    auto end = m_timers.lower_bound(sentry);
    
    // 【修改】使用 std::copy 无法直接转换类型，改用手动循环或 std::transform
    // 为了代码简洁易懂，我们直接用 range-based for loop 之前的迭代器循环
    for (auto it = m_timers.begin(); it != end; ++it)
    {
        expired.push_back(it->second);
    }

    // 删除过期的定时器
    m_timers.erase(m_timers.begin(), end);

    // 同时从 activeTimers 中移除
    for (const auto& it : expired)
    {
        ActiveTimer timer(it, it->sequence());
        m_activeTimers.erase(timer);
    }

    return expired;
}

void TimerQueue::reset(const std::vector<Timer*>& expired, Timestamp now)
{
    Timestamp nextExpire;

    for (const auto& timer : expired)
    {
        ActiveTimer activeTimer(timer, timer->sequence());
        
        // 【修改逻辑】
        //如果是重复定时器，且【不在】取消列表中，才重启
        if (timer->repeat() && 
            m_cancelingTimers.find(activeTimer) == m_cancelingTimers.end())
        {
            timer->restart(now);
            insert(timer);
        }
        else
        {
            // 否则（是一次性定时器，或者已经被取消了），直接删除
            delete timer; 
        }
    }

    if (!m_timers.empty())
    {
        nextExpire = m_timers.begin()->second->expiration();
        resetTimerfd(m_timerfd, nextExpire);
    }
}