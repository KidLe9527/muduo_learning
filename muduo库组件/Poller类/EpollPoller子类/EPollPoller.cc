#include <errno.h>
#include <unistd.h> // close
#include <string.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int kNew = -1;    // 某个channel还没添加至Poller          // channel的成员index_初始化为-1
const int kAdded = 1;   // 某个channel已经添加至Poller
const int kDeleted = 2; // 某个channel已经从Poller删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) 
    , events_(kInitEventListSize)   //  events_ = std::vector<epoll_event>(16)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller() 
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 由于频繁调用poll 实际上应该用LOG_DEBUG输出日志更为合理 当遇到并发场景 关闭DEBUG日志提升效率
    LOG_INFO("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;  // 保存errno的值，防止被后续代码修改
    Timestamp now(Timestamp::now());    // now是一个局部变量，生命周期仅限于poll函数内部，返回值是一个Timestamp对象，表示当前的时间戳，供EventLoop使用

    if (numEvents > 0)
    {
        LOG_INFO("%d events happend\n", numEvents); // LOG_DEBUG最合理
        fillActiveChannels(numEvents, activeChannels);  // 将发生事件的channel添加到activeChannels中，供EventLoop使用
        if (numEvents == events_.size()) // 扩容操作
        {
            events_.resize(events_.size() * 2); // 扩容为原来的两倍，避免频繁扩容带来的性能问题，扩容时会将新元素默认初始化为0（即epoll_event结构体的所有成员都被初始化为0），因此不需要手动清零
        }
    }
    else if (numEvents == 0)    // 超时返回0，正常情况，并不算错误，所以记录DEBUG日志即可
    {
        LOG_DEBUG("%s timeout!\n", __FUNCTION__);
    }
    else
    {
        if (saveErrno != EINTR) // EINTR表示epoll_wait被信号中断了，这种情况不算错误，其他错误才记录日志
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() error!");
        }
        // 真正错误只打日志不退出：是作者选择容错、保循环、不崩溃的服务器风格，不是漏写。
        // 典型的 Reactor 模型容错设计：尽量不崩溃，保持事件循环存活。
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index(); // index是channel在Poller中的状态，初始值为-1（kNew），表示还没有添加到Poller中，添加后设置为1（kAdded），删除后设置为2（kDeleted）
    LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;    // 将channel添加到channels_中，key是fd，value是channel指针，方便后续通过fd找到对应的channel
        }
        else // index == kDeleted   // 如果是已经删除的channel，说明之前已经添加过了，现在需要重新添加，所以不需要再添加到channels_中了，因为之前已经添加过了，channels_中已经有了这个channel的记录
        {
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel); // 调用update函数，传入操作类型EPOLL_CTL_ADD和channel指针，完成向epoll实例中添加fd的操作
    }
    else // channel已经在Poller中注册过了
    {
        if (channel->isNoneEvent()) // 如果channel不再关注任何事件了，就从epoll实例中删除这个fd，避免无效的fd占用epoll实例的资源
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel); // 否则就是修改这个fd的事件类型，调用update函数，传入操作类型EPOLL_CTL_MOD和channel指针，完成向epoll实例中修改fd的操作
        }
    }
}

// 从Poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);    // 移除时先删除映射，避免后续操作（如 epoll 事件触发）访问到已失效的 Channel 指针，防止野指针问题。

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的连接   // activeChannels是一个输出参数（eventloop调用poll函数时，传入一个空的vector，函数内部将发生事件的channel添加到这个vector中，供EventLoop使用）
// activeChannels = std::vector<Channel*>
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr); // events_[i].data是一个联合体，既可以通过data.fd访问，也可以通过data.ptr访问，这里我们使用data.ptr来存储Channel指针，避免了fd和Channel指针之间的转换，提高了效率
        channel->set_revents(events_[i].events);    // 设置发生的具体事件，events_[i].events是epoll_wait返回的事件类型，可能是EPOLLIN、EPOLLOUT等，Channel通过set_revents将这些事件类型保存起来，供EventLoop使用
        activeChannels->push_back(channel); // EventLoop就拿到了它的Poller给它返回的所有发生事件的channel列表了
    }
}

// 更新channel通道 其实就是调用epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    ::memset(&event, 0, sizeof(event)); // 将event结构体的所有成员都初始化为0，避免出现未初始化的成员导致的错误，尤其是联合体中的成员，因为联合体的所有成员共享同一块内存，如果不清零，可能会导致之前的值残留在内存中，影响后续的操作

    int fd = channel->fd();

    event.events = channel->events();
    //这里是联合体所以data.fd写了没有意义会被data.ptr覆盖
    // event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}