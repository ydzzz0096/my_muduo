# --- 这是我们为同步日志添加的目标 ---
add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE base_lib)

# --- [新增部分] 添加 LockQueue 的测试目标 ---
add_executable(test_lockqueue test_lockqueue.cpp)
# LockQueue 是纯头文件模板类，不依赖我们自己编译的 base_lib
# 但它使用了 std::thread，所以需要链接线程库
target_include_directories(test_lockqueue PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(test_lockqueue PRIVATE Threads::Threads)

# --- [新增部分] 添加线程的测试目标 ---
add_executable(test_thread test_thread.cpp)
target_link_libraries(test_thread PRIVATE base_lib)

# --- [新增部分] 添加线程池的测试目标 ---
add_executable(test_threadpool test_threadpool.cpp)
target_link_libraries(test_threadpool PRIVATE base_lib)

# 新增
add_executable(test_eventloop_v1 test_eventloop_v1.cpp)
target_link_libraries(test_eventloop_v1 PRIVATE net_lib)


add_executable(test_eventloop_v2 test_eventloop_v2.cpp)
target_link_libraries(test_eventloop_v2 PRIVATE net_lib)

add_executable(test_acceptor test_acceptor.cpp)
target_link_libraries(test_acceptor PRIVATE net_lib)

add_executable(test_timer test_timer.cpp)
target_link_libraries(test_timer PRIVATE net_lib)

add_executable(test_client test_client.cpp)
target_link_libraries(test_client PRIVATE net_lib)

add_executable(test_log_level test_log_level.cpp)
# 链接 base_lib 即可，不需要 net_lib
target_link_libraries(test_log_level PRIVATE base_lib)