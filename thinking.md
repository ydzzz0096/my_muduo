一:
    EventLoop、Poller、Channel 这“铁三角”，是 muduo 库的**“通用底层机制”**。
    Acceptor 和 TcpConnection 则是**“上层业务封装”**。
    Acceptor 和 TcpConnection 是 Channel 的“专业使用者”
    组合优于继承
    Channel 只负责事件分发，TcpConnection 只负责业务逻辑,低耦合
    Acceptor 封装了所有与**“接受新连接”**相关的逻辑。它拓展了什么: 它为 EventLoop 和 Channel 之间“EPOLLIN 事件”的传递，赋予了特定的含义——“新连接到来”
    TcpConnection 封装了所有与**“处理已建立连接”**相关的逻辑.它为 EventLoop 和 Channel 之间 EPOLLIN / EPOLLOUT 事件的传递，赋予了另一种特定的含义——“数据收发”