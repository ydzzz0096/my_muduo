# my_muduo

一个基于 Reactor 模式的高性能 C++11 网络库。
本项目是陈硕先生 `muduo` 网络库的核心重构与实现，旨在深入理解高性能网络编程的核心原理，如非阻塞 I/O、事件驱动循环、多线程模型以及现代 C++ 的工程实践。

## 🌟 核心特性 (Key Features)

* **底层模型**: 采用 **Epoll LT** (水平触发) + **非阻塞 I/O** (Non-blocking I/O)，高效处理高并发连接。
* **线程模型**: 实现了 **One Loop Per Thread** 模型。
    * **Main Reactor**: 主线程负责 `accept` 新连接。
    * **Sub Reactors**: I/O 子线程负责处理已连接 `socket` 的读写事件。
    * 利用 `EventLoopThreadPool` 进行线程管理和负载均衡（轮询分发）。
* **并发设施**:
    * 使用 `eventfd` 实现高效的**跨线程唤醒**机制，降低了锁竞争。
    * 实现了基于生产者-消费者模型的**任务队列**，支持跨线程任务派发 (`runInLoop`/`queueInLoop`)。
* **内存管理**:
    * 全面采用 **RAII** 机制管理资源（Socket, Thread, Mutex）。
    * 使用 `std::shared_ptr` 和 `std::weak_ptr` (结合 `tie` 机制) 解决多线程环境下的对象生命周期竞态条件。
* **高性能组件**:
    * **Buffer**: 实现了自动扩容的缓冲区，利用 `readv` 进行分散读，减少系统调用次数。
    * **AsyncLogging**: 双缓冲区异步日志系统，将日志写入 I/O 从业务线程中剥离，确保低延迟。

## 🏗️ 架构概览 (Architecture)

* **TcpServer**: 用户使用的直接接口 (Facade)，管理 Acceptor 和线程池。
* **EventLoop**: 事件循环核心，封装了 Poller (epoll) 和 Channel 分发机制。
* **Channel**: 封装 `fd` 及其感兴趣的事件 (Events) 和回调 (Callbacks)。
* **TcpConnection**: 封装一条 TCP 连接的完整生命周期，管理输入输出 Buffer。

## 🛠️ 构建与运行 (Build & Run)

本项目依赖 `CMake` 进行构建。

### 环境要求
* OS: Linux (推荐 Ubuntu/CentOS) 或 WSL2
* Compiler: g++ (支持 C++11)
* CMake

### 编译
```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 生成 Makefile
cmake ..

# 3. 编译 (生成静态库 libnet_lib.a 和测试程序)
make