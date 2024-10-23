#include "EchoServer.h"

EchoServer::EchoServer(const std::string &ip, const uint16_t port, int subthreadnum, int workthreadnum)
    :tcpserver_(ip, port, subthreadnum)
    ,threadpool_(workthreadnum, "WORKS")
{
    tcpserver_.setnewconnectioncb(std::bind(&EchoServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&EchoServer::HandleError, this, std::placeholders::_1));
    tcpserver_.setonmessagecb(std::bind(&EchoServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendcompletecb(std::bind(&EchoServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_.settimeoutcb(std::bind(&EchoServer::HandleTimeOut, this, std::placeholders::_1));
}

EchoServer::~EchoServer() {}

// 启动服务
void EchoServer::Start() { tcpserver_.start(); }

// 停止服务
void EchoServer::Stop()
{
    // 停止工作线程(为EchoServer的对象)
    threadpool_.stop();
    // printf("工作线程已停止\n");
    // 停止IO线程(事件循环)
    tcpserver_.stop();
}        

// 使用回调函数的方法让Connection对象和Accepter对象处于同一级，处理新客户端连接请求'
void EchoServer::HandleNewConnection(spConnection conn) 
{
    printf("%s new connection(fd=%d, ip=%s, port=%d) ok.\n", Timestamp::now().tostring().c_str(), conn->fd(), conn->ip().c_str(), conn->port());
    // 可以根据业务需要 增加其他的代码
}          

// 关闭客户端的链接 在Connection类中回调此函数
void EchoServer::HandleClose(spConnection conn) 
{
    printf("%s connection closed(fd=%d, ip=%s, port=%d) ok.\n", Timestamp::now().tostring().c_str(), conn->fd(), conn->ip().c_str(), conn->port());
    // 可以根据业务需要 增加其他的代码
}

// 客户端的链接错误 在Connection类中回调此函数
void EchoServer::HandleError(spConnection conn) {}          

// 数据发送完成后, 在Connection类中回调此函数
void EchoServer::HandleSendComplete(spConnection conn) {}        

// epoll_wait()超时, 在EventLoop类中回调此函数
void EchoServer::HandleTimeOut(EventLoop* loop) {} 

// 处理客户端的请求报文,在Connection中回调此函数
void EchoServer::HandleMessage(spConnection conn, std::string& message)
{
    if(threadpool_.size() == 0) 
    {
        // 若没有工作线程 直接用用IO线程处理业务 即直接处理
        OnMessage(conn, message);
    }
    else
        // 若有工作线程，把业务添加到工作线程池的任务队列中
        threadpool_.addtask(std::bind(&EchoServer::OnMessage, this, conn, message));
}

// 处理客户端的请求报文，用于添加给线程池
void EchoServer::OnMessage(spConnection conn, std::string& message)
{
    // printf("%s message (eventfd=%d): %s\n", Timestamp::now().tostring().c_str(), conn->fd(), message.c_str());

    // printf("EchoServer::OnMessage: %d\n", syscall(SYS_gettid));
    message = "replay: " + message;                  // 响应报文 回显业务
    conn->send(message.data(), message.size());
}                    