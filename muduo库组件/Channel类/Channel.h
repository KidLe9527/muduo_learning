#pragma once    // 防止头文件被重复包含，避免重复定义

#include <functional>   // for std::function，定义回调函数类型
#include <memory>   // for std::shared_ptr, std::weak_ptr

#include "noncopyable.h"    // for noncopyable基类
#include "Timestamp.h"  // for Timestamp时间戳类

class EventLoop;    // 前向声明 避免循环包含

/**
 * 理清楚 EventLoop、Channel、Poller之间的关系  Reactor模型上对应多路事件分发器
 * Channel理解为通道 封装了sockfd和其感兴趣的event 如EPOLLIN、EPOLLOUT事件 还绑定了poller返回的具体事件
 **/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>; // muduo仍使用typedef，普通事件回调函数类型，无参无返回值
    using ReadEventCallback = std::function<void(Timestamp)>;   // 读事件回调函数类型   

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到Poller通知以后 处理事件 handleEvent在EventLoop::loop()中调用 🔥
    // Poller 监听到某个 fd 有事件发生后，会通知对应的 EventLoop，EventLoop 再调用该 fd 绑定的 Channel 的 handleEvent 函数，由 Channel 完成最终的事件分发和回调执行。
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }   // 使用std::move提高效率,避免不必要的拷贝
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉 channel还在执行回调操作 🔥
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }          // 返回fd
    int events() const { return events_; }  // 返回fd感兴趣的事件
    void set_revents(int revt) { revents_ = revt; } // 设置fd发生的具体事件

    // 设置fd相应的事件状态 相当于epoll_ctl add delete
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }    // 判断fd是否正在关注写事件
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }  // 返回在Poller中的状态
    void set_index(int idx) { index_ = idx; }   // 设置在Poller中的状态

    // one loop per thread
    EventLoop *ownerLoop() { return loop_; }  // 返回所属的EventLoop
    void remove();  // 在所属的EventLoop中删除当前的channel // 手动调用remove删除，所以是public权限
private:

    void update();  // 更新channel所表示的fd的events事件到Poller中  // 每次更新events都要调用epoll_ctl内部修改，所以是private权限
    void handleEventWithGuard(Timestamp receiveTime);   // 事件处理函数的具体实现

    // 在头文件中是声明，需要在实现文件（.cpp） 中完成定义（初始化）
    static const int kNoneEvent;    
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd，Poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // Poller返回的具体发生的事件
    int index_;     // 在Poller中的状态，具体是表示该Channel在Poller中的位置或状态标识

    std::weak_ptr<void> tie_;   // 连接TcpConnection对象的生命周期
    bool tied_; // 标记是否连接TcpConnection对象的生命周期

    // 因为channel通道里可获知fd最终发生的具体的事件events，所以它负责调用具体事件的回调操作
    // 读事件需要携带 “事件发生的时间戳”，而其他事件不需要，这是由网络编程中不同事件的业务诉求和使用场景决定的。
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};