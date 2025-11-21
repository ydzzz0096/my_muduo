// examples/test_log_level.cpp

#include "base/Logger.h"
#include <iostream>
#include <unistd.h> // for sleep

int main()
{
    // 1. 默认情况（级别是 INFO）
    std::cout << "=== 1. Current Level: INFO (Default) ===" << std::endl;
    LOG_INFO << "This is an INFO log (Should appear)";
    LOG_ERROR << "This is an ERROR log (Should appear)";
    
    // 确保日志已经输出（因为是异步的，稍微等一下，或者手动sleep确保顺序）
    sleep(1); 
    std::cout << std::endl;

    // 2. 修改级别为 ERROR
    std::cout << "=== 2. Setting Level to ERROR... ===" << std::endl;
    Logger::getInstance().setLogLevel(ERROR);
    
    // 这条 INFO 日志应该被“静音”，不应该出现在屏幕上！
    LOG_INFO << "This is an INFO log (Should NOT appear!)"; 
    
    // 这条 ERROR 日志应该依然显示
    LOG_ERROR << "This is an ERROR log (Should appear)";

    sleep(1);
    std::cout << std::endl;

    // 3. 恢复级别为 INFO
    std::cout << "=== 3. Setting Level back to INFO... ===" << std::endl;
    Logger::getInstance().setLogLevel(INFO);
    
    LOG_INFO << "This is an INFO log (Should appear again)";

    return 0;
}