#ifndef LOGGER_H
#define LOGGER_H

#include "Timestamp.h"

#include <iostream>
#include <string>
#include <sstream> // 需要包含 stringstream
#include <functional>

// 日志级别
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

class LogStream; // 前向声明

// 日志类 (单例)
class Logger
{
public:
    static Logger& getInstance();

    // 设置日志级别
    void setLogLevel(LogLevel level);

    // 写日志，将格式化的日志消息写入到输出流中
    void log(const std::string& msg);
    
private:
    Logger() {}
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_logLevel;
};

// LogStream 类，用于构建流式日志
class LogStream
{
public:
    LogStream(const char* file, int line, LogLevel level);
    ~LogStream(); // 析构函数是关键

    // 重载 << 操作符
    template<typename T>
    LogStream& operator<<(const T& value)
    {
        m_buffer << value;
        return *this;
    }

private:
    std::ostringstream m_buffer;
};


// ----------------- 定义日志宏 -----------------
#define LOG_INFO  LogStream(__FILE__, __LINE__, INFO)
#define LOG_ERROR LogStream(__FILE__, __LINE__, ERROR)
#define LOG_FATAL LogStream(__FILE__, __LINE__, FATAL)
#define LOG_DEBUG LogStream(__FILE__, __LINE__, DEBUG)

#endif