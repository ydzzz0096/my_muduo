// net/InetAddress.cpp

#include "InetAddress.h"
#include <cstring>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    memset(&m_addr, 0, sizeof(m_addr));
    // 它在明确地告诉操作系统内核：“我这个地址结构体，使用的是 IPv4 协议族！”
    m_addr.sin_family = AF_INET;
    // 负责将 port 这个 16 位的短整型（Short）数字，从主机字节序（小端）转换成网络字节序（大端）
    m_addr.sin_port = htons(port);
    // 从字符串到网络字节序整数,并且已被inet_addr转换为网络字节序
    // s_addr真正用来存储 32 位二进制 IP 地址的整数
    m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(m_addr.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(m_addr.sin_port);
}