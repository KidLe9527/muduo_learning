#include <memory>

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "Logger.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop), name_(nameArg), started_(false), numThreads_(0), next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];    // 32 is enough for int32_t
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);  // 通过EventLoopThread类创建一个新的线程对象t，传入用户指定的回调函数cb和线程名称buf。这个线程对象t将负责在新线程中创建一个EventLoop对象，并运行事件循环。
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));    // 使用智能指针管理EventLoopThread对象的生命周期，确保在EventLoopThreadPool对象销毁时能够正确释放资源，避免内存泄漏
        loops_.push_back(t->startLoop()); // 底层创建线程 绑定一个新的EventLoop 并返回该loop的地址 （解释见底）
    }

    if (numThreads_ == 0 && cb) // 整个服务端只有一个线程运行baseLoop
    {
        cb(baseLoop_);  // 如果用户传入了回调函数cb，并且线程池中没有设置任何IO线程（numThreads_ == 0），则直接调用回调函数cb，传入主线程的EventLoop对象baseLoop_，供上层使用者在回调函数中对主线程的EventLoop进行初始化设置等操作
    }
}

// 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
EventLoop *EventLoopThreadPool::getNextLoop()
{
    // 如果只设置一个线程 也就是只有一个mainReactor 无subReactor 
    // 那么轮询只有一个线程 getNextLoop()每次都返回当前的baseLoop_
    EventLoop *loop = baseLoop_;    

    // 通过轮询获取下一个处理事件的loop
    // 如果没设置多线程数量，则不会进去，相当于直接返回baseLoop
    if(!loops_.empty())             
    {
        loop = loops_[next_];   // 先返回当前索引next_所指向的EventLoop对象的指针，然后将索引next_自增，准备返回下一个EventLoop对象的指针。通过轮询的方式分配Channel给subLoop，实现负载均衡。
        ++next_;
        // 轮询
        if(next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}



std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);  // 如果线程池中没有设置任何IO线程（loops_为空），则返回一个包含主线程的EventLoop对象baseLoop_的vector，供上层使用者在回调函数中对主线程的EventLoop进行初始化设置等操作
    }
    else
    {
        return loops_;  // 这里是返回的拷贝还是引用？ 这里返回的是loops_的拷贝，因为函数的返回类型是std::vector<EventLoop *>，而loops_是EventLoopThreadPool类的成员变量，返回时会调用vector的拷贝构造函数来创建一个新的vector对象，并将loops_中的元素复制到新的vector中，然后返回这个新的vector对象。虽然返回的是一个新的vector对象，但其中的元素（EventLoop指针）仍然指向原来的EventLoop对象，所以上层使用者在获取到这个vector后，可以通过其中的EventLoop指针来访问和操作原来的EventLoop对象。
    }
}


/*解释：loops_.push_back(t->startLoop());
这里通过eventloopthread对象 t 调用底层的std::thread创建一个新线程
然后在新线程中创建eventloop对象，用来处理事件循环，本函数最后返回的是这个eventloop的指针*/