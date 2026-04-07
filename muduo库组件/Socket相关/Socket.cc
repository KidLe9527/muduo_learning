#include <unistd.h> // close，作用是关闭一个文件描述符，释放系统资源
#include <sys/types.h>  // 这个头文件包含了系统数据类型的定义，如 ssize_t、off_t 等，这些类型在系统调用中经常使用。
#include <sys/socket.h> // 这个头文件包含了套接字编程相关的函数和数据结构的定义，如 socket()、bind()、listen()、accept() 等函数，以及 sockaddr_in 结构体等。
#include <string.h> // memset，作用是将一块内存区域的内容设置为指定的值，常用于初始化结构体或数组等数据结构。
#include <netinet/tcp.h>    // TCP_NODELAY，作用是提供TCP协议相关的选项，如TCP_NODELAY、TCP_CORK等，这些选项可以通过 setsockopt() 函数来设置，以控制TCP连接的行为。
#include <sys/socket.h> // SO_REUSEADDR，作用是提供套接字选项，如SO_REUSEADDR、SO_REUSEPORT等，这些选项可以通过 setsockopt() 函数来设置，以控制套接字的行为。

#include "Socket.h" // Socket类的定义
#include "Logger.h" // Logger类的定义，提供日志记录功能，用于输出错误信息和调试信息等。
#include "InetAddress.h"  // InetAddress类的定义，封装了IP地址和端口号等信息，提供了一些成员函数来获取和设置这些信息。

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail\n", sockfd_);    // LOG_FATAL是一个宏定义，作用是记录一个严重错误的日志，并且通常会终止程序的执行。它接受一个格式化字符串和可选的参数，就像printf函数一样，用于输出错误信息。在这个上下文中，当绑定套接字失败时，LOG_FATAL会记录一个包含套接字文件描述符的错误日志，并且可能会导致程序崩溃，以便开发者能够及时发现和修复问题。
    }
}

void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail\n", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     **/
    // accept4 只认识底层结构体 sockaddr_in / sockaddr，不认识封装的 InetAddress 类。
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    // fixed : int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)   // shutdown函数用于关闭套接字的写端，SHUT_WR表示关闭写端，允许套接字继续接收数据，但不再发送数据。如果函数返回值小于0，表示发生了错误，此时会记录一个错误日志，提示关闭写端失败。
    {
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    // TCP_NODELAY 用于禁用 Nagle 算法。
    // Nagle 算法用于减少网络上传输的小数据包数量。
    // 将 TCP_NODELAY 设置为 1 可以禁用该算法，允许小数据包立即发送。
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    // SO_REUSEADDR 允许一个套接字强制绑定到一个已被其他套接字使用的端口。
    // 这对于需要重启并绑定到相同端口的服务器应用程序非常有用。
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    // SO_REUSEPORT 允许同一主机上的多个套接字绑定到相同的端口号。
    // 这对于在多个线程或进程之间负载均衡传入连接非常有用。
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    // SO_KEEPALIVE 启用在已连接的套接字上定期传输消息。
    // 如果另一端没有响应，则认为连接已断开并关闭。
    // 这对于检测网络中失效的对等方非常有用。
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}