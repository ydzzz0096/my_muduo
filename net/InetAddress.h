// net/InetAddress.h

#ifndef INETADDRESS_H
#define INETADDRESS_H

#include <arpa/inet.h>// 提供了“网络地址”的转换工具（如 inet_addr, inet_ntop）
#include <netinet/in.h>// // 提供了基础数据类型（如 sockaddr_in）和常量（如 AF_INET）
#include <string>

// 封装 socket 地址类型
class InetAddress
{
public:
    // 易于使用的构造函数,内部会负责调用 htons 和 inet_addr 来完成到网络字节序的转换
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    // 方便从 accept 等系统调用返回的结果中创建 InetAddress 对象
    explicit InetAddress(const sockaddr_in &addr)
        : m_addr(addr)
    {}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in* getSockAddr() const { return &m_addr; }
    void setSockAddr(const sockaddr_in &addr) { m_addr = addr; }
    
private:
    sockaddr_in m_addr;
};

#endif