#pragma once

#include <functional>   // std::function，用于封装线程回调函数
#include <thread>
#include <memory>
#include <unistd.h>  // 用于获取线程ID等POSIX系统调用
#include <string>
#include <atomic>

#include "noncopyable.h"

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    bool started_;  // 线程是否已经启动
    bool joined_;   // 线程是否已经加入（等待线程结束）
    std::shared_ptr<std::thread> thread_;  // 底层线程对象，封装了实际的线程执行
    pid_t tid_;       // 在线程创建时再绑定，作用是在线程函数中获取线程ID，避免在start()函数中获取tid值时可能存在的竞争条件
    ThreadFunc func_; // 线程回调函数
    std::string name_;  // 线程名称，如果用户没有指定，则会自动生成一个默认名称，便于调试和日志记录
    static std::atomic_int numCreated_; // 记录已经创建的线程数量，用于生成默认线程名称
};