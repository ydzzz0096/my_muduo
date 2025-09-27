#include "Logger.h"

// === Logger 方法的实现 ===
Logger& Logger::getInstance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(LogLevel level)
{
    m_logLevel = level;
}

// 这个 log 方法是最终的输出口
void Logger::log(const std::string& msg)
{
    // 这里我们暂时只输出到标准输出
    std::cout << msg;
}

// === LogStream 方法的实现 ===
LogStream::LogStream(const char* file, int line, LogLevel level)
{
    // 预先格式化日志消息的头部
    m_buffer << Timestamp::now().toString() << " ";

    switch (level)
    {
    case INFO:
        m_buffer << "[INFO]";  // [修正] 去掉了多余的空格
        break;
    case ERROR:
        m_buffer << "[ERROR]";
        break;
    case FATAL:
        m_buffer << "[FATAL]";
        break;
    case DEBUG:
        m_buffer << "[DEBUG]";
        break;
    }

    // 输出文件名和行号
    m_buffer << " " << file << ":" << line << " ";
}

LogStream::~LogStream()
{
    // 在析构时，将缓冲区的内容加上换行符，并交给 Logger 实例处理
    m_buffer << "\n";
    Logger::getInstance().log(m_buffer.str());
}