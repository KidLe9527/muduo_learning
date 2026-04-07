#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    // 设置线程池中IO线程的数量（需在start()之前调用）
    // 1. numThreads = 0（默认值）：不创建任何IO线程，线程池无独立的EventLoop；
    //    所有事件循环任务均复用用户传入的baseLoop_（主线程的EventLoop），getNextLoop()始终返回baseLoop_；
    // 2. numThreads > 0：start()时会创建对应数量的EventLoopThread（IO线程），每个线程内部运行一个独立的EventLoop；
    //    这些新创建的EventLoop会存入loops_列表，baseLoop_仅作为主线程的EventLoop，不会被分配给任何子线程；
    // 3. 后续调用getNextLoop()时，会轮询返回loops_中的子线程EventLoop（numThreads>0时），实现多线程事件循环分发；
    void setThreadNum(int numThreads = 0) { numThreads_ = numThreads; } 

    void start(const ThreadInitCallback &cb = ThreadInitCallback()); // 启动线程池，创建IO线程并运行事件循环，调用该方法会根据之前设置的线程数量numThreads_创建对应数量的EventLoopThread（IO线程），每个线程内部运行一个独立的EventLoop，并调用用户传入的回调函数cb来对每个新创建的EventLoop进行初始化设置等操作，如果numThreads_为0，则直接在主线程的baseLoop_上调用回调函数cb进行初始化设置等操作

    // 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops(); // 获取所有的EventLoop

    bool started() const { return started_; } // 是否已经启动
    const std::string name() const { return name_; } // 获取名字

private:
    EventLoop *baseLoop_; // 用户使用muduo创建的loop 如果线程数为1 那直接使用用户创建的loop 否则创建多EventLoop
    std::string name_;//线程池名称，通常由用户指定，线程池中EventLoopThread名称依赖于线程池名称。
    bool started_;//是否已经启动标志
    int numThreads_;//线程池中线程的数量
    int next_; // 新连接到来，所选择EventLoop的索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_;//IO线程的列表
    std::vector<EventLoop *> loops_;//线程池中EventLoop的列表，指向的是EVentLoopThread线程函数创建的EventLoop对象。
};