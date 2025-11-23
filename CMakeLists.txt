# my_muduo/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(my_muduo VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置库文件的输出路径为项目根目录下的 lib 文件夹
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 查找线程库，如果找到，则创建一个名为 Threads::Threads 的目标
find_package(Threads REQUIRED)

# 将 base 子目录包含进来
add_subdirectory(base)

# 将 net 子目录包含进来
add_subdirectory(net)

# 将 examples 子目录包含进来
add_subdirectory(examples)


