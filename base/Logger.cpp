// base/Logger.cpp
// 实现日志输出的逻辑就是靠logger实例不断Pop(),需要的时候就会调用loggerstream的构造函数,构造出头部 信息,靠析构函数调用log和<<把buffer的内容push到队列.
// 异步日志系统：通过 LogStream (RAII, operator<<) 负责前端格式化并将日志提交给后端 Logger 单例，
// 实现了日志产生（快）与 I/O 输出（慢）的分离，确保调用线程的低延迟。
#include "Logger.h"
#include <iostream>

// === Logger 方法的实现 ===
Logger& Logger::getInstance()
{
    //静态全局变量 保证唯一
    static Logger logger;
    return logger;
}

// 被创造出来的时候就一直在寻找工作并完成
// 并且是唯一实例
Logger::Logger() : m_logLevel(INFO)
{
    m_writerThread = std::thread([this]() {
        while (true)
        {
            // Pop 现在返回一个 optional，可以判断是否有效
            auto result = m_logQueue.Pop();//wait 阻塞等待

            if (result.has_value())
            {
                // 如果 optional 有值，就取出值并打印
                std::cout << result.value(); 
            }
            else // 如果返回的是空 optional，说明队列已关闭且为空
            {
                break; // 收到“下班”信号，退出循环
            }
        }
    });
}

// 【核心重构】更新析构函数以匹配新的 LockQueue 接口
Logger::~Logger()
{
    // 只需调用队列的 Shutdown 方法
    // 调用shutdown之后lockqueue会停止接受新消息,唤醒所有消费者对消息输出,如果为空并停止就会返回nullopt
    m_logQueue.Shutdown();
    // 等待后台线程处理完所有剩余消息并安全退出
    if (m_writerThread.joinable())
    {
        m_writerThread.join();
    }
}


void Logger::setLogLevel(LogLevel level)
{
    m_logLevel = level;
}

void Logger::log(const std::string& msg)
{
    m_logQueue.Push(msg);
}

// === LogStream 的实现 (完全保持不变) ===
LogStream::LogStream(const char* file, int line, LogLevel level)
    :m_level(level)
{
    m_buffer << Timestamp::now().toString() << " ";
    switch (level)
    {
    case INFO:  m_buffer << "[INFO]";  break;
    case ERROR: m_buffer << "[ERROR]"; break;
    case FATAL: m_buffer << "[FATAL]"; break;
    case DEBUG: m_buffer << "[DEBUG]"; break;
    }
    m_buffer << " " << file << ":" << line << " ";
}

LogStream::~LogStream()
{
    m_buffer << "\n";
    Logger::getInstance().log(m_buffer.str());
    
    // 【新增】如果是 FATAL 级别，就终止程序
    if (m_level == FATAL)
    {
        abort(); // abort() 会立刻终止程序
    }
}
