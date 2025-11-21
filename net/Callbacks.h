    // net/Callbacks.h

    #ifndef CALLBACKS_H
    #define CALLBACKS_H

    #include <functional>
    #include <memory>

    /*
        Callbacks.h 是一个**“类型别名”的中央定义文件**。它通过集中管理所有回调函数的签名，为 net 库提供了一致的 API 接口，并巧妙地解决了 TcpServer 和 TcpConnection 之间可能发生的头文件循环依赖问题
    */
    class Buffer;
    class TcpConnection;
    class Timestamp;

    // 使用 std::shared_ptr 来管理 TcpConnection 的生命周期
    // 因为其生命周期被多方影响,至少会被tcpserver和echoserver持有
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&,
                                            Buffer*,
                                            Timestamp)>;

    #endif