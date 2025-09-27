# my_muduo/examples/CMakeLists.txt

# 添加头文件搜索路径，让 test_logger.cpp 能找到 "base/Logger.h"
include_directories(${CMAKE_SOURCE_DIR})

# 添加一个新的可执行文件目标
add_executable(test_logger test_logger.cpp)

# 关键：为 test_logger 这个目标，链接我们刚刚创建的 base_lib 库
target_link_libraries(test_logger PRIVATE base_lib)