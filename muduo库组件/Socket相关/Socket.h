#pragma once    // 防止头文件被多次包含，导致编译错误

#include "noncopyable.h"    // 非拷贝类，禁止拷贝构造和赋值构造

class InetAddress;  // 前向声明，避免包含InetAddress.h头文件，减少编译依赖

// 封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }

    ~Socket();

    int fd() const { return sockfd_; }
    
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);
    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};
