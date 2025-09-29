# my_muduo/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(my_muduo VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找线程库，如果找到，则创建一个名为 Threads::Threads 的目标
find_package(Threads REQUIRED)

# 将 base 子目录包含进来
add_subdirectory(base)

# 将 examples 子目录包含进来
add_subdirectory(examples)