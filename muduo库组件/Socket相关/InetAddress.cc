#include <strings.h>
#include <string.h>

#include "InetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    ::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = ::htons(port); // 本地字节序转为网络字节序 // ：：的作用是调用全局函数，避免和成员函数重名
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());    // 更好的做法是inet_pton，支持IPV6：：inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);  // c_str()将string转为const char*
}

// 作用是把网络字节序的IP地址转换为点分十进制字符串形式
std::string InetAddress::toIp() const
{
    // addr_
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

// 作用是把网络字节序的IP地址和端口号转换为点分十进制字符串和十进制端口号的组合形式，格式为 "ip:port"
std::string InetAddress::toIpPort() const
{
    // ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = ::strlen(buf); // 获取IP地址字符串的长度，作为端口号字符串的起始位置
    uint16_t port = ::ntohs(addr_.sin_port);    // 网络字节序转为本地字节序，获取端口号。port是一个16位的无符号整数，表示端口号的值
    sprintf(buf+end, ":%u", port);  // 将端口号格式化为字符串，并追加到IP地址字符串的末尾，形成 "ip:port" 的格式。%u表示无符号整数，buf+end表示从IP地址字符串的末尾开始写入端口号字符串
    return buf;
}

// 作用是把网络字节序的端口号转换为本地字节序的端口号，并返回这个端口号的值
uint16_t InetAddress::toPort() const
{
    return ::ntohs(addr_.sin_port); // 网络字节序转为本地字节序，获取端口号。ntohs函数的作用是将一个16位的无符号整数从网络字节序转换为主机字节序。网络字节序是一种统一的字节顺序，用于在不同类型的计算机之间传输数据，而主机字节序则是当前计算机使用的字节顺序。通过调用ntohs函数，可以确保在不同平台上正确地解析端口号。
}

#if 0
#include <iostream>
int main()
{
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;  // 输出：127.0.0.1:8080
}
#endif