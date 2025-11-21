// base/Logger.h

#ifndef LOGGER_H
#define LOGGER_H

#include "Timestamp.h"
#include "LockQueue.h"
#include <string>
#include <thread>
#include <sstream>//三个重要的类 ostringstream istringstream stringstream
#include <functional>
/*
这个头文件的作用:
std::function (最重要的)

它是什么: 一个通用的、多态的函数包装器。

核心作用: 它可以存储、复制和调用任何满足特定函数签名的可调用实体。无论是普通函数指针、Lambda 表达式、bind 表达式、还是类的成员函数（需要配合对象实例），只要它们的参数列表和返回值类型匹配,std::function 都能“装”下。

std::bind

它是什么: 一个函数适配器。

核心作用: 它可以将一个可调用实体（比如成员函数）与其部分或全部参数（包括 this 指针）**“绑定”**在一起，生成一个新的、参数可能更少的可调用对象。

*/
// 日志级别 (保持不变)
enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

// 日志类 (单例模式)
class Logger
{
public:
    static Logger& getInstance();
    void setLogLevel(LogLevel level);
    void log(const std::string& msg);
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_logLevel;
    LockQueue<std::string> m_logQueue;
    std::thread m_writerThread;
};

// LogStream 和 日志宏 (完全保持不变)
class LogStream
{
public:
    LogStream(const char* file, int line, LogLevel level);
    ~LogStream();

    template<typename T>
    //operator<< 运算符重载，实际上就是把所有传入的数据，都转发给了它内部的一个 std::ostringstream 成员
    LogStream& operator<<(const T& value)
    {
        m_buffer << value;
        return *this;
    }

private:
    std::ostringstream m_buffer;
    LogLevel m_level;
};

#define LOG_INFO  LogStream(__FILE__, __LINE__, INFO)
#define LOG_ERROR LogStream(__FILE__, __LINE__, ERROR)
#define LOG_FATAL LogStream(__FILE__, __LINE__, FATAL)
#define LOG_DEBUG LogStream(__FILE__, __LINE__, DEBUG)

#endif