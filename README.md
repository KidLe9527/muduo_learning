# muduo_learning · C++ Reactor 网络库学习

这个仓库记录我学习 muduo 风格 TCP 网络库的过程，包含一套可构建的核心实现、按组件拆分的源码笔记、基础示例和连接测试截图。重点是理解 Linux 网络编程中事件分发、线程模型、连接生命周期和缓冲区之间的关系。

代码以 muduo 和相关学习项目为参考。仓库中的参考实现、个人注释和练习代码共同组成学习记录；本文描述的是当前源码已有的功能。

## 从哪里开始

| 路径 | 内容 |
| --- | --- |
| [`AAA_muduo-core-main/`](AAA_muduo-core-main/) | 完整工程入口：CMake、头文件、实现文件和 EchoServer 示例 |
| [`AAA_muduo-core-main/include/`](AAA_muduo-core-main/include/) | 网络库接口与核心类声明 |
| [`AAA_muduo-core-main/src/`](AAA_muduo-core-main/src/) | Reactor、连接管理、线程、缓冲区等实现 |
| [`muduo库组件/`](muduo库组件/) | 按类拆分的学习源码，适合对照阅读 |
| [`zdemo集/`](zdemo集/) | 独立的基础练习，未接入主工程构建 |
| [`images/`](images/) | 客户端连接、断开和服务端日志截图 |

根目录的 `testserver.cc` 是独立示例文件；CMake 实际构建的入口是 [`AAA_muduo-core-main/example/testserver.cc`](AAA_muduo-core-main/example/testserver.cc)。

## 核心工作流程

```text
客户端建立连接
  → 主 EventLoop 检测监听 socket 可读
  → Acceptor 接收连接
  → TcpServer 创建 TcpConnection，轮询选择工作 EventLoop
  → 工作线程注册 Channel，交给 EPollPoller 监听
  → 可读事件 → 输入 Buffer → 用户消息回调
  → send → 直接写 socket / 写入输出 Buffer，等待可写事件继续发送
```

- `EventLoop`：每个 I/O 线程运行一个事件循环，分发活跃事件并执行待处理任务。
- `Channel` 与 `EPollPoller`：封装 fd 的事件关注与回调，通过 Linux `epoll` 收集就绪事件。
- `EventLoopThreadPool`：提供工作线程；主循环接受连接，工作循环处理连接 I/O。
- `eventfd`：唤醒事件循环，让跨线程投递的任务及时执行。
- `TcpConnection` 与 `Buffer`：管理连接状态、读写缓冲和发送完成等回调。

当前示例设置了 **3 个工作 I/O 线程**，监听 **8080 端口**，将收到的数据原样发回客户端。

## 编译与运行

环境要求：Linux、支持 C++11 的编译器、CMake、pthread。源码依赖 `epoll`、`eventfd` 等 Linux 接口，应在 Linux 主机、虚拟机或容器中构建。

在仓库根目录执行：

```bash
cmake -S AAA_muduo-core-main -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j
./AAA_muduo-core-main/example/testserver
```

`CMAKE_POLICY_VERSION_MINIMUM` 用于兼容保留了旧版最低版本声明的工程与 CMake 4。现有 CMake 会把动态库输出到 `AAA_muduo-core-main/lib/`，把示例输出到 `AAA_muduo-core-main/example/`。

另开终端发送消息：

```bash
nc 127.0.0.1 8080
```

输入一行文本，应收到相同内容；服务端会记录连接建立和断开的日志。仓库内已提交的可执行文件与本机环境可能不同，运行前应重新编译。

## 阅读顺序与当前边界

建议按 `testserver.cc → TcpServer → Acceptor → EventLoop/Channel/EPollPoller → TcpConnection → Buffer → EventLoopThreadPool` 的顺序阅读，再对照 `muduo库组件/` 查看注释和练习。

这是用于理解 TCP 网络库的学习实现。当前主工程提供 EchoServer 示例，尚未包含 HTTP 协议层、完整定时器系统或自动化测试集，也没有可复现的吞吐量基准。跨线程发送时的数据生命周期、错误处理和连接关闭边界仍值得继续核查。

## 参考与延伸

- [陈硕的 muduo](https://github.com/chenshuo/muduo)
- 《Linux 多线程服务端编程：使用 muduo C++ 网络库》
- 工程内保留的 [muduo-core 参考说明](AAA_muduo-core-main/README.md)
- [个人学习博客](https://kidle9527.github.io/)
