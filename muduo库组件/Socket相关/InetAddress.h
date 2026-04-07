#pragma once

#include <arpa/inet.h>  // inet_addr, inet_ntop
#include <netinet/in.h> // sockaddr_in
#include <string>

// 封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");

    // explicit 保护：避免误触发隐式转换（比如把数字直接当成 InetAddress 对象传参）
    // 作用是把系统原生的 sockaddr_in 结构体直接封装成 InetAddress 对象，这个是转换构造函数，不是拷贝构造函数，因为参数类型不同
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {
    }

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in *getSockAddr() const { return &addr_; }   // 这个函数的作用是返回一个指向 InetAddress 类的成员变量 addr_ 的常量指针，允许外部代码访问 InetAddress 对象所封装的 socket 地址信息，但不允许修改它。
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; } // 这个函数的作用是将一个 sockaddr_in 结构体对象作为参数传入，并将其赋值给 InetAddress 类的成员变量 addr_，从而更新 InetAddress 对象所封装的 socket 地址信息。

private:
    sockaddr_in addr_;
};