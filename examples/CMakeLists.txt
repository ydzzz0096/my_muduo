# my_muduo/examples/CMakeLists.txt

add_executable(echoserver echoserver.cpp)
target_link_libraries(echoserver PRIVATE net_lib)

add_executable(test_qps test_qps.cpp)
target_link_libraries(test_qps PRIVATE net_lib)