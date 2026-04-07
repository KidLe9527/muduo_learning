#pragma once

#include <vector>
#include <string>
#include <algorithm>    // std::copy
#include <stddef.h>     // for size_t，因为size_t是一个无符号整数类型，通常用于表示对象的大小或数组的索引等。它在C++标准库中定义，并且在不同的平台上可能具有不同的大小（通常是32位或64位）。因此，包含stddef.h头文件可以确保我们能够正确使用size_t类型。

// 网络库底层的缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;//初始预留的prependabel空间大小
    static const size_t kInitialSize = 1024;//初始缓冲区大小，是一个常量，表示Buffer对象在创建时默认分配的缓冲区大小为1024字节（1KB）。这个值可以根据实际需求进行调整，以适应不同应用场景的性能要求和内存使用情况。

    explicit Buffer(size_t initalSize = kInitialSize)
        : buffer_(kCheapPrepend + initalSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {
    }

    ~Buffer() = default;

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }    // readerIndex_之前的空间是prependable的 也就是已经读过的空间 可以被丢弃掉的空间
    const char *peek() const { return begin() + readerIndex_; } // 返回缓冲区中可读数据的起始地址

    // 从缓冲区中读取len长度的数据 也就是把readerIndex_向前移动len个字节
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len; // 说明应用只读取了可读缓冲区数据的一部分，就是len长度 还剩下readerIndex+=len到writerIndex_的数据未读
        }
        else // len == readableBytes()
        {
            retrieveAll();
        }
    }

    // 从缓冲区中读取所有数据 也就是把readerIndex_和writerIndex_都复位到kCheapPrepend位置
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 从缓冲区中读取所有数据 转成string类型的数据返回
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }

    // 从缓冲区中读取len长度的数据 转成string类型的数据返回
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);    // 起始地址 + 长度 读取从readerIndex_开始的len长度的数据
        retrieve(len); // 上面一句把缓冲区中可读的数据已经读取出来 这里肯定要对缓冲区进行复位操作
        return result;
    }

    // buffer_.size - writerIndex_  // buffer_末尾剩余的可写空间大小
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容
        }
    }

    // 把[data, data+len]内存上的数据添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());    // 把data指向的内存上len长度的数据复制到beginWrite()指向的内存上 也就是writerIndex_位置开始的内存上
        writerIndex_ += len;
    }

    char *beginWrite() { return begin() + writerIndex_; }   // 返回缓冲区中可写数据的起始地址 也就是writerIndex_位置的地址
    const char *beginWrite() const { return begin() + writerIndex_; }   // 相比上面那个函数 这个函数是const版本的 也就是说当Buffer对象是const类型的时候 就会调用这个函数 返回一个const char*类型的指针 以保证不能通过这个指针修改缓冲区中的数据

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    // vector底层数组首元素的地址 也就是数组的起始地址
    char *begin() { return &*buffer_.begin(); } // &*buffer_.begin() 这个表达式的意思是先获取vector底层数组的迭代器（buffer_.begin()），然后解引用这个迭代器得到数组的第一个元素（*buffer_.begin()），最后取这个元素的地址（&*buffer_.begin()）。这样就得到了vector底层数组的起始地址。
    const char *begin() const { return buffer_.data(); }    // .data()函数返回vector底层数组的起始地址 这个函数是C++11引入的 之前的版本中没有这个函数 所以只能通过&*buffer_.begin()来获取底层数组的地址

    // 确保buffer_中有足够的空间容纳len长度的数据 如果没有就进行扩容
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) // 如果所有空闲加起来都不够放 len 字节 + 前面要留 8 字节 → 必须直接扩容！
        {
            buffer_.resize(writerIndex_ + len); // 这里为什么不直接resize到buffer_.size() + len呢？buffer_.size() + len 是盲目追加容量（既浪费空间，也不符合 “按需扩容” 的设计逻辑）。
        }
        else // 这里说明 len <= xxx + writer 把reader搬到从xxx开始 使得xxx后面是一段连续空间
        {
            size_t readable = readableBytes(); // 当前缓冲区中可读数据的长度
            // 将当前缓冲区中从readerIndex_到writerIndex_的数据 （即可读数据） 拷贝到缓冲区起始位置kCheapPrepend处，以便腾出更多的可写空间
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};
