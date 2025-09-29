// base/Logger.h

#ifndef LOGGER_H
#define LOGGER_H

#include "Timestamp.h"
#include "LockQueue.h" // 1. 包含我们实现的线程安全队列
#include <string>
#include <thread>      // 2. 包含线程头文件
#include <sstream>
#include <functional>
#include <atomic>      // 3. 包含原子操作头文件


// 日志级别 (保持不变)
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

// 日志类 (单例)
class Logger
{
public:
    // 获取唯一的实例对象
    static Logger& getInstance();

    // 设置日志级别
    void setLogLevel(LogLevel level);

    // 3. log方法的新职责：将日志消息放入阻塞队列
    void log(const std::string& msg);
    
private:
    Logger(); // 构造函数需要实现，以启动后台线程
    ~Logger(); // 析构函数需要实现，以等待后台线程结束
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    // 2. 新增一个退出标志
    std::atomic<bool> m_exit_flag; 

    // 4. 新增的成员变量
    LogLevel m_logLevel;
    LockQueue<std::string> m_logQueue; // 日志缓冲队列
    std::thread m_writerThread; // 后台写日志的线程
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
};

#define LOG_INFO  LogStream(__FILE__, __LINE__, INFO)
#define LOG_ERROR LogStream(__FILE__, __LINE__, ERROR)
#define LOG_FATAL LogStream(__FILE__, __LINE__, FATAL)
#define LOG_DEBUG LogStream(__FILE__, __LINE__, DEBUG)

#endif