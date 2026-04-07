#include "Thread.h"
#include "CurrentThread.h"  // 获取线程ID等线程相关信息的工具类

#include <semaphore.h>  // 信号量，用于线程同步，确保线程创建后获取到tid值

std::atomic_int Thread::numCreated_(0); // 初始化静态成员变量，记录已经创建的线程数量，初始值为0

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    // 创建线程时，设置默认名称（如果用户没有指定），并且增加线程计数器，确保每个线程都有一个唯一的默认名称
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)   // 如果线程已经启动但没有加入（等待线程结束），则分离线程，避免资源泄漏
    {
        thread_->detach();  // 设置分离线程，线程运行后自动销毁（非阻塞）
    }
}

void Thread::start()  // 一个Thread对象 记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;  // 定义一个信号量，用于线程同步，确保线程创建后获取到tid值
    sem_init(&sem, false, 0);  // 初始化信号量，初始值为0，表示线程创建后还未获取到tid值。 false表示该信号量在进程内共享，适用于线程间同步。 true表示在多个进程间共享，适用于进程间同步。                                        
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        tid_ = CurrentThread::tid();    // 获取线程的tid值, 这里获取的是新线程的tid值，而不是主线程的tid值，因为这个lambda函数是在新线程中执行的，所以CurrentThread::tid()会返回新线程的tid值。
        sem_post(&sem); // 线程创建成功后，释放信号量，通知主线程可以获取tid值了
        func_();    // 在新线程中执行用户传入的线程回调函数，用户的业务逻辑就在这个函数中实现
    }));

    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem); // 等待信号量，直到新线程创建成功并获取到tid值后，主线程才能继续执行，确保在start()函数返回之前tid_已经被正确设置，避免竞争条件
    sem_destroy(&sem); // 销毁信号量，释放资源
}

// C++ std::thread 中join()和detach()的区别：https://blog.nowcoder.net/n/8fcd9bb6e2e94d9596cf0a45c8e5858a
void Thread::join()
{
    joined_ = true;
    thread_->join();    // 阻塞当前线程，直到新线程执行完毕，确保资源正确回收，避免内存泄漏
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;    // 先自增再使用，确保每个线程的编号唯一且从1开始，是原子操作，避免多线程环境下的竞争条件
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num); //（sizeof 变量名）->获取缓冲区的大小，可以不加括号
        name_ = buf;
    }
}