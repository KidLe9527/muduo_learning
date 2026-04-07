#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)  // 通过Thread类创建的线程将运行EventLoopThread::threadFunc()方法。这里只是创建Thread类对象，没有真正创建线程，真正创建线程是在调用startLoop()方法时通过调用Thread类对象的start()方法来创建的
    , mutex_()
    , cond_()   // 条件变量的默认构造函数会初始化一个空的条件变量对象，这里可以不写
    , callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)   // 如果loop_不为nullptr，说明线程已经创建成功并且正在运行事件循环，此时需要退出事件循环并等待线程结束，确保资源正确回收，避免内存泄漏
    {
        loop_->quit();
        thread_.join();
    }
}

// 这个方法是在主线程中调用的 用于启动线程 创建一个新的EventLoop对象 并返回该loop的地址
EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启用底层线程Thread类对象thread_中通过start()创建的线程，该线程将运行EventLoopThread::threadFunc()方法，创建一个EventLoop对象，并调用其loop()方法开启事件循环

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this](){return loop_ != nullptr;});   // 等待条件变量，直到新线程创建成功并获取到EventLoop对象的指针后，主线程才能继续执行，确保在startLoop()函数返回之前loop_已经被正确设置，避免竞争条件
        loop = loop_;
    }
    return loop; 
}

// 下面这个方法 是在单独的新线程里运行的，创建一个EventLoop对象，并调用其loop()方法开启事件循环
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的EventLoop对象 和上面的线程是一一对应的 即one loop per thread

    if (callback_)  // 这个函数的作用是让上层使用者在新线程中创建EventLoop对象后对EventLoop进行初始化设置等操作，提供了一个灵活的接口，供上层使用者在回调函数中对EventLoop进行初始化设置等操作
    {
        callback_(&loop);   // 调用上层注册的回调函数callback_ 传入当前线程的EventLoop对象指针 // 上层使用者在回调函数中对EventLoop进行初始化设置等操作
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;   // 将当前线程的EventLoop对象的指针赋值给成员变量loop_，供主线程使用。s
        cond_.notify_one(); // 通知主线程，当前线程已经创建成功并获取到EventLoop对象的指针了，主线程可以继续执行了
    }
    loop.loop();    // 执行EventLoop的loop() 开启了底层的Poller的poll()
    std::unique_lock<std::mutex> lock(mutex_);  // 在退出线程之前，获取互斥锁，确保对loop_的访问是线程安全的
    loop_ = nullptr;
}


/*EventLoopThread的核心职责是 “线程创建 + EventLoop 绑定”，把 “线程创建、loop 初始化、线程安全的句柄传递” 封装成统一接口，
上层无需关心线程和 loop 的底层创建逻辑

EventLoopThread 管理新线程，在新线程中创建的eventloop将loop的引用传递给主线程*/