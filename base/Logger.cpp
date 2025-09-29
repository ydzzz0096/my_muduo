// base/Logger.cpp

#include "Logger.h"
#include <iostream>

// === Logger 方法的实现 ===
Logger& Logger::getInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger() : m_exit_flag(false)
{
    m_writerThread = std::thread([this]() {
        while (true)
        {
            std::string msg = this->m_logQueue.Pop();
            
            // 收到退出信号后，在打印完队列中剩余的所有消息后退出
            if (this->m_exit_flag && msg.empty())
            {
                break;
            }

            std::cout << msg;
        }
    });
    // 【重要】不再 detach 线程！我们需要在析构时 join 它。
}

Logger::~Logger()
{
    m_exit_flag = true;
    m_logQueue.Push(""); 
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

// === LogStream 的实现 ===
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