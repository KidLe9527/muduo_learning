#include <sys/types.h>  // socket，bind，listen，accept等系统调用需要的头文件
#include <sys/socket.h> // socket，bind，listen，accept等系统调用需要的头文件
#include <errno.h>  // 错误码相关的头文件
#include <unistd.h> // close系统调用需要的头文件

#include "Acceptor.h"
#include "Logger.h" // 日志相关的头文件
#include "InetAddress.h"    // InetAddress类，封装了IP地址和端口号的相关操作

// 创建一个非阻塞的socket，并设置CLOEXEC标志，防止子进程继承该文件描述符
// static：这个函数只在当前 .cpp 文件内可见，外部不能调用，是内部工具函数。
static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);    // 最后把一个参数可以传入0吗？ 当然可以，0表示使用默认协议，对于SOCK_STREAM类型的socket，默认协议就是IPPROTO_TCP，所以传入0也是可以的。但是为了代码的清晰和可读性，明确指定协议类型是一个更好的选择。这里我们使用IPPROTO_TCP来明确表示我们创建的是一个TCP socket，这样其他开发者在阅读代码时能够更快地理解我们的意图。
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);   // 文件名、函数名、行号和错误码
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport = true)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    // TcpServer::start() => Acceptor.listen() 如果有新用户连接 要执行一个回调(accept => connfd => 打包成Channel => 唤醒subloop)
    // baseloop监听到有事件发生 => acceptChannel_(listenfd) => 执行该回调函数

    // 强绑定——> 把 “成员函数” 和 “所属对象” 绑定成一个独立的可调用实体，让 Channel 能在触发事件时，准确找到对应的 Acceptor 对象并执行它的 handleRead 方法。
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();    // 移除Poller中注册的所有事件
    acceptChannel_.remove();        // 调用EventLoop->removeChannel => Poller->removeChannel 把Poller的ChannelMap对应的部分删除
    listenning_ = false;            // 显式置为false，清理状态标记
    // acceptSocket_ 会自动析构，关闭监听fd；
    // acceptChannel_ 会自动析构（无额外操作，核心注销已主动完成）
}

void Acceptor::listen()
{
    if(listenning_)
    {
        return; // 已经在监听了，直接返回
    }
    acceptSocket_.listen();         // listen
    acceptChannel_.enableReading(); // acceptChannel_注册至Poller !重要
    listenning_ = true;            // 标记为正在监听
}

// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;   // 定义一个InetAddress对象，用于存储新连接的对端地址信息
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd, peerAddr); // 轮询找到subLoop 唤醒并分发当前的新客户端的Channel
        }
        else
        {
            ::close(connfd);    // 如果没有设置回调函数，说明没有地方处理这个新连接，所以直接关闭它，避免资源泄漏。
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno); // 记录accept失败的错误日志，包含文件名、函数名、行号和错误码，帮助开发者排查问题。
        if (errno == EMFILE)    // 进程文件描述符达到上限，无法接受新的连接了，这时可以记录一个特殊的错误日志，提示开发者检查系统的文件描述符限制设置，或者优化服务器的连接处理逻辑，避免过多的文件描述符被占用。
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}