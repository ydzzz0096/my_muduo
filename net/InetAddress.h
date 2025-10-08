// net/InetAddress.h

#ifndef INETADDRESS_H
#define INETADDRESS_H

#include <arpa/inet.h>// 提供“网络地址”的转换工具
#include <netinet/in.h>// 提供基础数据类型和常量
#include <string>

// 封装 socket 地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
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