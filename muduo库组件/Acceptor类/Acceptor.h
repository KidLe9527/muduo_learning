#pragma once    // 防止头文件被重复包含

#include <functional>   // std::function，用于定义回调函数类型

#include "noncopyable.h"    // 继承自noncopyable类，禁止拷贝和赋值操作
#include "Socket.h"     // Socket类，封装了socket相关的系统调用和操作
#include "Channel.h"    // Channel类，封装了文件描述符和事件的关系，以及事件的回调函数

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>; // 接收新连接的回调函数类型，参数为新连接的socket文件描述符和对端地址信息

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();
    
    //设置新连接的回调函数
    void setNewConnectionCallback(const NewConnectionCallback &cb) { NewConnectionCallback_ = cb; }
    // 判断是否在监听
    bool listenning() const { return listenning_; }
    // 监听本地端口
    void listen();

private:
    void handleRead();//处理新用户的连接事件

    EventLoop *loop_; // Acceptor用的就是用户定义的那个baseLoop 也称作mainLoop
    Socket acceptSocket_;//专门用于接收新连接的socket
    Channel acceptChannel_;//专门用于监听新连接的channel
    NewConnectionCallback NewConnectionCallback_;//新连接的回调函数
    bool listenning_;//是否在监听
};