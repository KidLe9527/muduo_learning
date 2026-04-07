#pragma once            // 防止头文件被多次包含，导致重复定义错误

#include <memory>       // std::shared_ptr，智能指针，提供自动内存管理功能，允许多个指针共享同一资源，并在最后一个指针销毁时自动释放资源
#include <functional>   // std::function，函数对象，允许将函数、lambda表达式、绑定表达式等封装成可调用对象

class Buffer;
class TcpConnection;
class Timestamp;

// 1. TcpConnection 智能指针类型：muduo 中 TcpConnection 生命周期由智能指针管理
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

// 2. 连接建立回调：Tcp 连接建立时触发，参数为连接对象指针
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;

// 3. 连接关闭回调：Tcp 连接关闭时触发
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;

// 4. 写完成回调：数据成功写入内核缓冲区后触发（并非对方收到）
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;

// 5. 高水位标记回调：输出缓冲区数据量超过阈值时触发（用于流量控制）
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

// 6. 消息到达回调：收到对方数据时触发，参数包含连接对象、数据缓冲区、时间戳
using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;