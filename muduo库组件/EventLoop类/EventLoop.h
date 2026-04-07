#pragma once

#include <functional>   // std::function，是C++11引入的一个模板类，用于封装任何可调用对象（函数、函数对象、lambda表达式等），使其具有统一的调用接口。
#include <vector>
#include <atomic>   // std::atomic_bool，C++11引入的原子类型，用于实现线程安全的布尔变量，避免了使用互斥锁来保护布尔变量的访问。
#include <memory>   // std::unique_ptr，C++11引入的智能指针，用于管理动态分配的对象，确保对象在不再使用时自动释放内存，避免内存泄漏。
#include <thread>   // std::thread，C++11引入的线程库，用于创建和管理线程，提供了跨平台的线程支持。
#include <mutex>    // std::mutex，C++11引入的互斥锁，用于保护共享资源的访问，确保在多线程环境下数据的一致性和安全性。

#include "noncopyable.h"    // noncopyable是一个基类，禁止派生类对象被复制和赋值，通常通过删除拷贝构造函数和赋值运算符来实现。
#include "Timestamp.h"  // Timestamp类，表示时间戳，提供了获取当前时间戳、将时间戳转换为字符串等功能。
#include "CurrentThread.h"  // CurrentThread命名空间，提供获取当前线程内核ID的功能； 使用 __thread 线程局部存储缓存tid，避免频繁系统调用，提高性能

class Channel;
class Poller;

// 事件循环类 主要包含了两个大模块 Channel Poller(epoll的抽象)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;  // 无参无返回值的回调函数类型

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollRetureTime_; } // 返回Poller返回发生事件的Channels的时间点

    // 在当前loop中执行
    void runInLoop(Functor cb);
    // 把上层注册的回调函数cb放入队列中 唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);

    // 通过eventfd唤醒loop所在的线程
    void wakeup();

    // EventLoop的方法 => Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程里
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); } // threadId_为EventLoop创建时的线程id CurrentThread::tid()为当前线程id

private:
    void handleRead();        // 给eventfd返回的文件描述符wakeupFd_绑定的事件回调 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait
    void doPendingFunctors(); // 执行上层回调

    std::atomic_bool looping_; // 原子操作 底层通过CAS实现。 标识当前loop是否在运行
    std::atomic_bool quit_;    // 标识退出loop循环。 通过在loop循环中检查该标志来决定是否退出循环
    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作

    const pid_t threadId_; // 记录当前EventLoop是被哪个线程id创建的 即标识了当前EventLoop的所属线程id
    Timestamp pollRetureTime_; // Poller返回发生事件的Channels的时间点


    std::unique_ptr<Poller> poller_;    // EventLoop通过一个独占的智能指针持有一个Poller对象，负责IO事件的监控和分发

    int wakeupFd_; // 作用：当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 通过该成员唤醒subLoop处理Channel
    std::unique_ptr<Channel> wakeupChannel_;    // 作用：绑定wakeupFd_，当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait

    using ChannelList = std::vector<Channel *>;
    ChannelList activeChannels_; // 返回Poller检测到当前有事件发生的所有Channel列表

    std::vector<Functor> pendingFunctors_;    // 存储loop需要执行的所有回调操作
    std::mutex mutex_;                        // 互斥锁 用来保护上面vector容器的线程安全操作
};