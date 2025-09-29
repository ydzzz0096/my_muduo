# my_muduo/base/CMakeLists.txt

# 将 base 目录下的所有源文件编译成一个名为 "base_lib" 的静态库
add_library(base_lib STATIC
    Logger.cpp
    Timestamp.cpp
    Thread.cpp
)

target_include_directories(base_lib INTERFACE 
    ${CMAKE_SOURCE_DIR}
)
