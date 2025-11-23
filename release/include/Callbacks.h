// net/Callbacks.h

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <functional>
#include <memory>

class Buffer;
class TcpConnection;
class Timestamp;

// 使用 std::shared_ptr 来管理 TcpConnection 的生命周期
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&,
                                           Buffer*,
                                           Timestamp)>;
                                           
using TimerCallback = std::function<void()>;

#endif