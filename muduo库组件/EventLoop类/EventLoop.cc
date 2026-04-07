#include <sys/eventfd.h>    // eventfd函数的头文件，提供了创建事件文件描述符的接口
#include <unistd.h>         // 提供了对POSIX操作系统API的访问，包括read、write等函数
#include <fcntl.h>          // 提供了文件控制选项的接口，如O_NONBLOCK等常量
#include <errno.h>          // 提供了对错误码的访问，如errno变量和相关的错误码常量
#include <memory>           // std::unique_ptr，C++11引入的智能指针，用于管理动态分配的对象，确保对象在不再使用时自动释放内存，避免内存泄漏。

#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"

// 防止一个线程创建多个EventLoop,因为一个线程只能有一个EventLoop实例。__thread是线程局部存储，每个线程都有自己的t_loopInThisThread变量，互不干扰。
__thread EventLoop *t_loopInThisThread = nullptr;   // t_loopInThisThread用于记录当前线程是否已经创建了一个EventLoop实例，如果已经创建了，就不能再创建第二个EventLoop实例，否则会导致程序逻辑错误。

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000; // 10000毫秒 = 10秒钟

/* 创建线程之后主线程和子线程谁先运行是不确定的。
 * 通过一个eventfd在线程之间传递数据的好处是多个线程无需上锁就可以实现同步。
 * eventfd支持的最低内核版本为Linux 2.6.27,在2.6.26及之前的版本也可以使用eventfd，但是flags必须设置为0。
 * 函数原型：
 *     #include <sys/eventfd.h>
 *     int eventfd(unsigned int initval, int flags);
 * 参数说明：
 *      initval,初始化计数器的值。
 *      flags, EFD_NONBLOCK,设置socket为非阻塞。
 *             EFD_CLOEXEC，执行fork的时候，在父进程中的描述符会自动关闭，子进程中的描述符保留。
 * 场景：
 *     eventfd可以用于同一个进程之中的线程之间的通信。
 *     eventfd还可以用于同亲缘关系的进程之间的通信。
 *     eventfd用于不同亲缘关系的进程之间通信的话需要把eventfd放在几个进程共享的共享内存中（没有测试过）。
 */
// 创建wakeupfd 用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);   //  Linux 提供的轻量事件通知 fd，只用来做线程唤醒，不传输真实数据。
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d\n", errno); // Logger是muduo库提供的日志系统，LOG_FATAL表示记录一个严重错误的日志，并且通常会终止程序的运行。这里记录了创建eventfd失败的错误码，帮助开发者定位问题。
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())   // 后面所有操作都会检查：只能在这个线程里操作这个 loop
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())    // 创建一个 eventfd，专门用来唤醒阻塞在 epoll_wait 的线程，这是跨线程唤醒 loop 的唯一手段
    , wakeupChannel_(new Channel(this, wakeupFd_))  // 创建一个 Channel，专门用来监听 wakeupFd_ 上的可读事件，当有事件发生时，调用 EventLoop::handleRead() 来处理这个事件。 this 指向当前的 EventLoop 对象，wakeupFd_ 是之前创建的 eventfd 文件描述符。
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    
    // 设置wakeupfd的回调事件类型以及发生事件后的回调操作 🔥🔥
    wakeupChannel_->setReadCallback( 
        // 这里绑定的读事件回调函数，channel中定义的读事件回调函数类型是ReadEventCallback，参数是Timestamp类型的事件发生时间戳，而EventLoop::handleRead()函数没有参数，所以需要使用std::bind来适配这个回调函数的参数。
        // 因为bind会把this指针绑定到EventLoop::handleRead函数的隐含this参数上，所以生成的可调用对象是一个无对象依赖的函数对象，可以直接作为回调函数使用。而timestamp参数在调用时会被自动传入，所以不需要在bind中指定占位符。
        std::bind(&EventLoop::handleRead, this)); // 把 this（当前 EventLoop 对象的指针）提前绑定到成员函数的隐含this参数上，生成一个 “无对象依赖” 的可调用对象。
    
    wakeupChannel_->enableReading(); // 每一个EventLoop都将监听wakeupChannel_的EPOLL读事件了 ｜ 设置fd相应的events事件状态
}

// 先停止事件 → 再移除监听 → 再关 fd → 最后解绑线程
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll(); // 给Channel移除所有感兴趣的事件
    wakeupChannel_->remove();     // 把Channel从EventLoop上删除掉
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n", this);

    while (!quit_)
    {
        activeChannels_.clear();    // 清空上一次事件循环中发生事件的Channel列表，准备接收新一轮事件循环中发生事件的Channel列表
        pollRetureTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生了事件 然后上报给EventLoop 通知channel处理相应的事件
            channel->handleEvent(pollRetureTime_);
        }
        /**
         * 执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程 mainloop(mainReactor) 主要工作：
         * accept接收连接 => 将accept返回的connfd打包为Channel => TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
         *
         * mainloop调用queueInLoop将回调加入subloop（该回调需要subloop执行 但subloop还在poller_->poll处阻塞） queueInLoop通过wakeup将subloop唤醒
         **/

        // 这是跨线程、异步任务执行点，执行别的线程塞给这个 loop 的回调函数。
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

/**
 * 退出事件循环
 * 1. 如果loop在自己的线程中调用quit成功了 说明当前线程已经执行完毕了loop()函数的poller_->poll并退出
 * 2. 如果不是当前EventLoop所属线程中调用quit退出EventLoop 需要唤醒EventLoop所属线程的epoll_wait
 *
 * 比如在一个subloop(worker)中调用mainloop(IO)的quit时 需要唤醒mainloop(IO)的poller_->poll 让其执行完loop()函数
 *
 * ！！！ 注意： 正常情况下 mainloop负责请求连接 将回调写入subloop中 通过生产者消费者模型即可实现线程安全的队列
 * ！！！       但是muduo通过wakeup()机制 使用eventfd创建的wakeupFd_ notify 使得mainloop和subloop之间能够进行通信
 **/
void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在当前loop中执行cb
/*不管你在哪个线程调用 runInLoop (cb)，它都能保证 cb 一定在这个 EventLoop 所属的线程里安全执行。*/
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 当前EventLoop中执行回调
    {
        cb();
    }
    else // 在非当前EventLoop线程中执行cb，就需要唤醒EventLoop所在线程执行cb
    {
        queueInLoop(cb);
    }
}

// 把cb放入队列中 唤醒loop所在的线程执行cb
/*
queueInLoop 是 EventLoop 核心方法之一，跨线程提交任务的核心入口
核心作用是将指定的回调函数（Functor）加入当前 EventLoop 的待执行任务队列，
并在必要时唤醒 EventLoop 所在线程，确保回调能被及时执行。*/
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);  // 这里为什么用unique_lock而不是lock_guard？ 因为unique_lock提供了更多的灵活性，比如可以在需要的时候手动解锁和重新锁定，而lock_guard在作用域结束时会自动释放锁，无法手动控制锁的生命周期。
        pendingFunctors_.emplace_back(cb);  // 1. 🔥🔥加锁把任务放进队列，这个队列是当前eventloop的成员变量，也就是这个 EventLoop 所绑定的那个线程的任务队列。所有线程都可以访问，所以需要加锁保护线程安全
    }

    /**
     * || callingPendingFunctors的意思是 当前loop正在执行回调中 但是loop的pendingFunctors_中又加入了新的回调 需要通过wakeup写事件
     * 唤醒相应的需要执行上面回调操作的loop的线程 让loop()下一次poller_->poll()不再阻塞（阻塞的话会延迟前一次新加入的回调的执行），然后
     * 继续执行pendingFunctors_中的回调函数
     **/
    if (!isInLoopThread() || callingPendingFunctors_)   // 如果不是当前EventLoop线程中调用queueInLoop 或者当前loop正在执行回调中 但是loop的pendingFunctors_中又加入了新的回调 就需要唤醒loop所在线程执行cb
    {
        wakeup(); // // 2. 满足条件就唤醒loop所在线程
    }
}

/*handleRead 是绑定到 wakeupChannel_ 的读事件回调，其核心逻辑就是读取并清空 eventfd 的计数器。
handleRead 负责 “清理唤醒的痕迹”，保证线程唤醒后能回到正常的 IO 复用循环，同时为下一次唤醒做准备*/
void EventLoop::handleRead()
{
    uint64_t one = 1;   // 用来接收数据的8字节临时变量
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
    }
}

// 用来唤醒loop所在线程 向wakeupFd_写一个数据 wakeupChannel就发生读事件 当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;   // eventfd的计数器是一个64位无符号整数，写入一个值会增加计数器的值，读出一个值会将计数器的值重置为0。这里写入1表示增加计数器的值，唤醒等待在这个eventfd上的线程。
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

// EventLoop的方法 => Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

// bool EventLoop::hasChannel(Channel *channel)
// {
//     return poller_->hasChannel(channel);
// }


// 执行上层回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;  // 定义一个局部变量functors来存储需要执行的回调函数，使用swap的方式将pendingFunctors_中的回调函数交换到functors中，这样可以减少锁的持有时间，提高效率，同时避免了死锁的风险。
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); // 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁 | pendingFunctors_中的回调函数被交换到functors中后，pendingFunctors_就变成了一个空的vector，这样在执行functor()时，如果有新的回调函数通过queueInLoop()加入到pendingFunctors_中，就不会影响正在执行的functors中的回调函数，也不会导致死锁。
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}
