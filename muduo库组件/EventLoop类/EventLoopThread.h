#pragma once

#include <functional>   // std::function，是C++11引入的一个模板类，用于封装可调用对象（函数、函数对象、lambda表达式等）。
#include <mutex>
#include <condition_variable>
#include <string>

#include "noncopyable.h"
#include "Thread.h"

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    // 在EventLoopThread类中，ThreadInitCallback是一个std::function类型的别名，表示一个接受EventLoop指针参数并返回void的回调函数类型。
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop(); // 启动线程并返回一个指向EventLoop对象的指针，供上层使用，调用该方法会创建一个新线程，在新线程中运行EventLoopThread::threadFunc()方法，创建一个EventLoop对象，并调用其loop()方法开启事件循环

private:
    void threadFunc();  // 线程函数，在线程中运行，创建一个EventLoop对象，并调用其loop()方法开启事件循环

    EventLoop *loop_;   // 指向当前线程的EventLoop对象的指针，初始值为nullptr，表示尚未创建EventLoop对象
    bool exiting_;  // 标志位，表示是否正在退出线程，初始值为false，表示线程尚未退出
    Thread thread_; // 封装了底层的线程创建和管理，通过Thread类创建的线程将运行EventLoopThread::threadFunc()方法
    std::mutex mutex_;             // 互斥锁
    std::condition_variable cond_; // 条件变量
    ThreadInitCallback callback_;   // 用户注册的回调函数，在新线程中创建EventLoop对象后会调用该回调函数，传入当前线程的EventLoop对象指针，供上层使用者在回调函数中对EventLoop进行初始化设置等操作
};