# my_muduo/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(my_muduo VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 新增这一行：将 base 子目录包含进来，生成 base_lib
add_subdirectory(base)

# 将 examples 子目录包含进来
add_subdirectory(examples)