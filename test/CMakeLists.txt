# 测试目录的CMake配置

# 添加基准测试可执行文件
add_executable(benchmark_test benchmark_test.cpp)

# 链接库
target_link_libraries(benchmark_test
    net_lib
    base_lib
    pthread
)

# 设置头文件包含路径
target_include_directories(benchmark_test PRIVATE
    ${PROJECT_SOURCE_DIR}/net
    ${PROJECT_SOURCE_DIR}/base
)

# 将可执行文件输出到bin目录
set_target_properties(benchmark_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)