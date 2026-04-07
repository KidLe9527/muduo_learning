#include <errno.h>  
#include <sys/uio.h>    // readv/writev，是一个系统调用，允许在一次调用中对多个缓冲区进行读写操作。它使用一个iovec结构数组来描述多个缓冲区，每个iovec结构包含一个指向缓冲区的指针和该缓冲区的长度。readv从文件描述符读取数据并将其分散到多个缓冲区中，而writev则将多个缓冲区中的数据聚集起来写入文件描述符。这种方式可以减少系统调用的次数，提高效率，特别是在需要处理大量数据时。
#include <unistd.h>   // read/write系统调用，提供了对文件描述符进行读写操作的接口。read函数从文件描述符中读取数据并将其存储在指定的缓冲区中，而write函数则将指定缓冲区中的数据写入文件描述符。这些函数是Unix/Linux系统中进行输入输出操作的基本工具，广泛用于处理文件、网络套接字等各种类型的I/O资源。

#include "Buffer.h"

/**
 * 从fd上读取数据 Poller工作在LT模式
 * Buffer缓冲区是有大小的！ 但是从fd上读取数据的时候 却不知道tcp数据的最终大小
 *
 * @description: 从socket读到缓冲区的方法是使用readv先读至buffer_，
 * Buffer_空间如果不够会读入到栈上65536个字节大小的空间，然后以append的
 * 方式追加入buffer_。既考虑了避免系统调用带来开销，又不影响数据的接收。
 **/
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    // 栈额外空间，用于从套接字往出读时，当buffer_暂时不够用时暂存数据，待buffer_重新分配足够空间后，在把数据交换给buffer_。
    char extrabuf[65536] = {0}; // 栈上内存空间 65536/1024 = 64KB

    /*
    struct iovec {
        ptr_t iov_base; // iov_base指向的缓冲区存放的是readv所接收的数据或是writev将要发送的数据
        size_t iov_len; // iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度
    };
    */

    // 使用iovec分配两个连续的缓冲区
    struct iovec vec[2];
    const size_t writable = writableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小 不一定能完全存储从fd读出的数据

    // 第一块缓冲区，指向Buffer内置缓冲区的可写区域，长度为当前可写字节数
    vec[0].iov_base = begin() + writerIndex_;   // 也就是beginWrite()函数返回的地址 也就是writerIndex_位置的地址
    vec[0].iov_len = writable;
    // 第二块缓冲区，指向栈空间上的临时缓冲区，长度是64KB
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    // 这里之所以说最多128k-1字节，是因为若writable为64k-1，那么需要两个缓冲区 第一个64k-1 第二个64k 所以做多128k-1
    // 如果第一个缓冲区>=64k 那就只采用一个缓冲区 而不使用栈空间extrabuf[65536]的内容
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;   // 如果Buffer的可写空间小于64KB 就使用两个缓冲区 否则就只使用一个缓冲区 因为Buffer的可写空间已经足够存储从fd读出的数据了 就不需要使用栈空间了
    const ssize_t n = ::readv(fd, vec, iovcnt); // readv系统调用从fd上读取数据到vec数组描述的缓冲区中；readv 不是 “同时写”，是 “顺序写，自动填满一个再填下一个”。

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else // extrabuf里面也写入了n-writable长度的数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable); // 对buffer_扩容 并将extrabuf存储的另一部分数据追加至buffer_
    }
    return n;
}

/*该函数未处理 “部分写入” 场景（即n > 0但n < readableBytes()）：非阻塞 IO 下，write可能只写入部分数据，此时需要上层记录已写入的长度，下次继续写剩余数据；
仅封装了基础的write调用，未做缓冲区收缩、指针移动（readerIndex_的移动需上层根据返回值n手动处理）。
*/
ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}


// inputBuffer_.readFd表示将对端数据读到inputBuffer_中，移动writerIndex_指针
// outputBuffer_.writeFd标示将数据写入到outputBuffer_中，从readerIndex_开始，可以写readableBytes()个字节