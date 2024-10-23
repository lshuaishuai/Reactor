#pragma once

#include "TcpServer.h"
#include "EventLoop.h"
#include "Connection.h"
#include "ThreadPool.h"

// 回显服务器
class EchoServer
{
public:
    EchoServer(const std::string &ip, const uint16_t port, int subthreadnum = 3, int workthreadnum=5);
    ~EchoServer();

    void Start();       // 启动服务
    void Stop();        // 停止服务
    
    void HandleNewConnection(spConnection conn);                   // 使用回调函数的方法让Connection对象和Accepter对象处于同一级，处理新客户端连接请求
    void HandleClose(spConnection conn);                           // 关闭客户端的链接 在Connection类中回调此函数
    void HandleError(spConnection conn);                           // 客户端的链接错误 在Connection类中回调此函数
    void HandleMessage(spConnection conn, std::string& message);   // 处理客户端的请求报文,在Connection中回调此函数
    void HandleSendComplete(spConnection conn);                    // 数据发送完成后, 在Connection类中回调此函数
    void HandleTimeOut(EventLoop* loop);                          // epoll_wait()超时, 在EventLoop类中回调此函数
    void OnMessage(spConnection conn, std::string& message);       // 处理客户端的请求报文，用于添加给线程池
private:
    TcpServer tcpserver_;
    ThreadPool threadpool_;         // 工作线程池
};
