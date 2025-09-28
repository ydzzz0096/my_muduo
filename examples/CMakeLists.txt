# my_muduo/examples/CMakeLists.txt

# 添加头文件搜索路径，以便能找到 "base/..."
include_directories(${CMAKE_SOURCE_DIR})

# --- 这是我们为同步日志添加的目标 ---
add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE base_lib)

# --- [新增部分] 添加 LockQueue 的测试目标 ---
add_executable(test_lockqueue test_lockqueue.cpp)
# LockQueue 是纯头文件模板类，不依赖我们自己编译的 base_lib
# 但它使用了 std::thread，所以需要链接线程库
target_link_libraries(test_lockqueue PRIVATE Threads::Threads)