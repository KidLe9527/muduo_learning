#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"    // 防止类被拷贝和赋值
#include "Timestamp.h"

class Channel;
class EventLoop;    // 前向声明，事件循环

// muduo库中多路事件分发器的核心IO复用模块
class Poller
{
public:
    using ChannelList = std::vector<Channel *>; // 定义Channel指针的动态数组类型

    Poller(EventLoop *loop);
    virtual ~Poller() = default;    // 虚析构函数，确保派生类析构时能正确调用

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0; // 作用是等待IO事件发生，并将发生事件的Channel添加到activeChannels中
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前的Poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现 🔥
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    // map的key:sockfd value:sockfd所属的channel通道类型    
    // protected: 允许派生类访问，但不允许外部访问
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
};


// Poller* Poller::newDefaultPoller(EventLoop* loop) {
//     // 1. 检测系统是否支持 epoll（Linux 特有）
//     // _SC_EPOLL 是系统配置项，通过 sysconf 获取
//     if (::sysconf(_SC_EPOLL)) { 
//         // 优先返回高性能的 EPollPoller 实例
//         return new EPollPoller(loop);
//     } else {
//         // 降级为 PollPoller（兼容所有 POSIX 系统）
//         return new PollPoller(loop);
//     }
// }

// 这个函数的实现放在 Poller.cc 中
// 传入的参数 loop 是 Poller 所属的 EventLoop 对象
// new xxxPoller 根据系统环境选择合适的 Poller 实现类（如 EPollPoller 或 PollPoller）
// 返回值是一个指向新创建的 Poller 对象的指针
// new xxx(EventLoop*) 会调用相应 Poller 实现类的构造函数，传入所属的 EventLoop 对象