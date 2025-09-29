#include "base/Logger.h"

int main()
{
    LOG_INFO << "这是我们提供的API接口的中文注释。";
    int number = 42;
    LOG_ERROR << "An error occurred! Error code: " << number;
    
    LOG_FATAL << "A fatal error occurred! System is shutting down.";
    
    // 只有在编译时定义了 DEBUG 宏，LOG_DEBUG 才会有用
    #ifdef DEBUG
    LOG_DEBUG << "This is a debug message.";
    #endif

    //std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}