#pragma once

#include "TcpServer.h"
#include "EventLoop.h"
#include "Connection.h"
#include "ThreadPool.h"

/*
    BankServer类：网上银行服务器
*/

class UserInfo  // 用户信息(状态机)
{
private:
    int fd_;            // 客户端的fd
    std::string ip_;    // 客户端的ip地址
    bool login_;        // 客户端登陆的状态：true-已登录；false-未登录
    std::string name_;  // 客户端的用户名
public:
    UserInfo(int fd, const std::string &ip)
        :fd_(fd)
        ,ip_(ip)
    {}
    void setLogin(bool login) { login_ = login; }
    bool Login() { return login_; }
};

// 回显服务器
class BankServer
{
public:
    BankServer(const std::string &ip, const uint16_t port, int subthreadnum = 3, int workthreadnum=5);
    ~BankServer();

    void Start();       // 启动服务
    void Stop();        // 停止服务
    
    void HandleNewConnection(spConnection conn);                   // 使用回调函数的方法让Connection对象和Accepter对象处于同一级，处理新客户端连接请求
    void HandleClose(spConnection conn);                           // 关闭客户端的链接 在Connection类中回调此函数
    void HandleError(spConnection conn);                           // 客户端的链接错误 在Connection类中回调此函数
    void HandleMessage(spConnection conn, std::string& message);   // 处理客户端的请求报文,在Connection中回调此函数
    void HandleSendComplete(spConnection conn);                    // 数据发送完成后, 在Connection类中回调此函数
    void HandleTimeOut(EventLoop* loop);                           // epoll_wait()超时, 在EventLoop类中回调此函数
    void OnMessage(spConnection conn, std::string& message);       // 处理客户端的请求报文，用于添加给线程池
    void HandleRemove(int fd);                                     // 客户端连接超时时，在TcpServer中回调这个成员函数
private:
    using spUserInfo = std::shared_ptr<UserInfo>;
    TcpServer tcpserver_;
    ThreadPool threadpool_;                // 工作线程池
    std::mutex mutex_;                     // 保护状态机的互斥锁
    std::map<int, spUserInfo> usermap_;    // 用户的状态机
};
