#include <stdlib.h> // getenv，getenv函数用于获取环境变量的值，入参是环境变量名（字符串），返回值是指向环境变量值的字符串指针；若环境变量不存在，返回 nullptr。
#include "Poller.h"
#include "EPollPoller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL")) // MUDUO_USE_POLL 是一个自定义环境变量，用户可以通过设置这个环境变量来强制使用 poll 作为 IO 复用的实现；如果这个环境变量存在（不关心值），说明用户希望使用 poll，那么就返回一个 PollPoller 的实例；否则，默认使用性能更好的 EPollPoller 实例。
    {
        return nullptr; // 生成poll的实例
    }
    else
    {
        return new EPollPoller(loop); // 生成epoll的实例
    }
}

/*是否使用 poll/epoll仅取决于环境变量是否存在，和环境变量的具体值（比如设为 1/0/true/ 空字符串等）完全无关。*/