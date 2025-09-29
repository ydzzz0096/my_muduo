// base/Logger.cpp

#include "Logger.h"
#include <iostream>

// === Logger 方法的实现 ===
Logger& Logger::getInstance()
{
    static Logger logger;
    return logger;
}

// 【核心修正】更新构造函数中的后台线程逻辑
Logger::Logger()
{
    m_writerThread = std::thread([this]() {
        while (true)
        {
            // Pop 现在返回一个 optional，可以判断是否有效
            auto result = m_logQueue.Pop();

            if (result.has_value())
            {
                // 如果 optional 有值 (has_value() == true)，就取出值并打印
                // .value() 方法可以获取 optional 中存储的实际值
                std::cout << result.value(); 
            }
            else // 如果返回的是空 optional (std::nullopt)，说明队列已关闭且为空
            {
                break; // 收到“下班”信号，退出循环
            }
        }
    });
}

// 【核心修正】更新析构函数以匹配新的 LockQueue 接口
Logger::~Logger()
{
    // 关闭队列，这将唤醒后台线程
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
