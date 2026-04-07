#include <string>

#include "TcpServer.h"
#include "Logger.h"

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置合适的subloop线程数量
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }

private:
    // 连接建立或断开的回调函数
    void onConnection(const TcpConnectionPtr &conn)   
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        // conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }
    TcpServer server_;
    EventLoop *loop_;

};

int main() {
    EventLoop loop;
    InetAddress addr(8080);
    EchoServer server(&loop, addr, "EchoServer");
    server.start(); // 启动服务器，监听端口并等待客户端连接 // 为什么不先开启loop再开启服务器？ 因为服务器的启动过程包括了Acceptor的listen()方法，而listen()方法会注册监听事件到mainloop的Poller中，如果先开启loop()，mainloop就会进入事件循环，等待事件发生，但此时Acceptor还没有注册监听事件，所以mainloop的事件循环就无法处理新连接事件了，导致服务器无法正常工作。因此应该先启动服务器，注册监听事件，然后再开启事件循环，这样才能保证服务器能够正确处理客户端连接和消息。
    loop.loop();    // 启动事件循环，处理客户端连接和消息，这里的loop是mainloop，负责监听新连接事件，分发给subloop处理，而subloop负责处理已连接的客户端的读写事件
    return 0;
}