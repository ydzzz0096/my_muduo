// base/Logger.h

#ifndef LOGGER_H
#define LOGGER_H

#include "Timestamp.h"
#include "LockQueue.h"
#include <string>
#include <thread>
#include <sstream>
#include <functional>

enum LogLevel {
    DEBUG, // 0
    INFO,  // 1
    ERROR, // 2
    FATAL, // 3
};

// 日志类 (单例)
class Logger
{
public:
    static Logger& getInstance();
    void setLogLevel(LogLevel level);

    int logLevel() const { return m_logLevel; }

    void log(const std::string& msg);
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_logLevel;
    LockQueue<std::string> m_logQueue;
    std::thread m_writerThread;
    // std::atomic<bool> m_exit_flag; // 【移除】不再需要这个退出标志
};

// LogStream 和 日志宏 (完全保持不变)
class LogStream
{
public:
    LogStream(const char* file, int line, LogLevel level);
    ~LogStream();

    template<typename T>
    LogStream& operator<<(const T& value)
    {
        m_buffer << value;
        return *this;
    }

private:
    std::ostringstream m_buffer;
    LogLevel m_level;// 【新增】记录当前消息的级别
};

// 【修改后】增加判断逻辑
#define LOG_INFO \
    if (Logger::getInstance().logLevel() <= INFO) \
        LogStream(__FILE__, __LINE__, INFO)

#define LOG_DEBUG \
    if (Logger::getInstance().logLevel() <= DEBUG) \
        LogStream(__FILE__, __LINE__, DEBUG)

#define LOG_ERROR \
    if (Logger::getInstance().logLevel() <= ERROR) \
        LogStream(__FILE__, __LINE__, ERROR)

#define LOG_FATAL \
    if (Logger::getInstance().logLevel() <= FATAL) \
        LogStream(__FILE__, __LINE__, FATAL)

#endif