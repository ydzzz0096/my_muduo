// base/Logger.cpp

#include "Logger.h"
#include <iostream>

// === Logger 方法的实现 ===
Logger& Logger::getInstance()
{
    static Logger logger;
    return logger;
}

// 【核心重构】更新构造函数中的后台线程逻辑
Logger::Logger() : m_logLevel(INFO)
{
    m_writerThread = std::thread([this]() {
        while (true)
        {
            // Pop 现在返回一个 optional，可以判断是否有效
            auto result = m_logQueue.Pop();

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
}


// === Timestamp 的实现 (完全保持不变) ===
#include "Timestamp.h"
#include <chrono>
#include <ctime>

Timestamp::Timestamp() : m_microSecondsSinceEpoch(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : m_microSecondsSinceEpoch(microSecondsSinceEpoch) {}

Timestamp Timestamp::now()
{
    return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string Timestamp::toString() const
{
    char buf[128] = {0};
    time_t seconds = m_microSecondsSinceEpoch / 1000000;
    tm tm_time;
    localtime_r(&seconds, &tm_time);

    snprintf(buf, 128, "%04d-%02d-%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec);
    return buf;
}