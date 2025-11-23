#!/bin/bash

set -e

# 清理旧的 build
rm -rf `pwd`/build
mkdir `pwd`/build

# 编译
cd `pwd`/build
cmake ..
make
cd ..

# 创建 release 目录结构
rm -rf `pwd`/release
mkdir -p `pwd`/release/include
mkdir -p `pwd`/release/lib

# 拷贝头文件
# 将 base 和 net 下的 .h 文件都拷过去，保持扁平化或者保留目录结构均可
# 这里我们简单粗暴，全部拷到 include 下
cp base/*.h release/include/
cp net/*.h  release/include/

# 拷贝库文件
# 我们的 CMakeLists.txt 修改后，库文件应该在 lib/ 目录下
cp lib/*.a release/lib/

echo "Build done! Headers and libs are in `pwd`/release"