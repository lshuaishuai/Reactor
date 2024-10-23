#pragma once

#include <sys/epoll.h>
#include <functional>
#include <memory>

#include "Epoll.h"
#include "InetAddress.h"
#include "Socket.h"
#include "EventLoop.h"

class Epoll;
class EventLoop;

class Channel
{
public:
    Channel(EventLoop* loop, int fd);   // Channel是Accepter和Connection的下层类
    ~Channel();

    int fd();
    void useet();                  // 采用ET模式
    void enablereading();          // 让epoll_wait监视fd_的读事件, 注册读事件
    void disablereading();         // 取消读事件
    void enablewriting();          // 让epoll_wait监视fd_的写事件, 注册写事件
    void disablewriting();         // 取消写事件
    void disableall();             // 取消全部的事件
    void remove();                 // 从事件循环中删除全部的Channel
    void setinepoll();             // 把inepoll_设置为true
    void setrevents(uint32_t ev);  // 设置revents_成员的值为参数ev
    bool inepoll();                // 返回inepoll_
    uint32_t events();             // 返回events_的值
    uint32_t revents();            // 返回revents_的值

    void handleevent();             // 事件处理函数 epoll_wait返回时,执行它

    // void onmessage();                                // 处理对端发过来的消息
    void setreadcallback(std::function<void()> fn);  // 设置fd_读事件的回调函数 
    
    void setclosecallback(std::function<void()> fn); // 设置fd_关闭(断开)的回调函数，将调用Connection::closecallback()
    void seterrorcallback(std::function<void()> fn); // 设置fd_错误的回调函数， 将调用Connection::errorcallback() 
    void setwritecallback(std::function<void()> fn); // 设置写事件的回调函数

private:
    int fd_ = -1;                // Channel拥有的fd,Channel和fd是一对一的关系
    // Epoll *ep_ = nullptr;     // Channel对应的红黑树,channel与Epoll是多对一的关系,一个Channel只对应一个Epoll
    EventLoop* loop_ = nullptr;  // 是从外面传进来的
    bool inepoll_= false;        // Channel是否已添加到epoll红黑树上,如果未添加,调用epoll_ctl()的时候用EPOLL_CTL_ADD,否则用EPOLL_CTL_MOD
    uint32_t events_ = 0;        // fd_需要监视的事件, listenfd和clientfd需要监视EPOLLIN, clientfd也可能需要监视EPOLLOUT
    uint32_t revents_ = 0;       // fd_已发生的事件

    std::function<void()> readcallback_;    // fd_读事件的回调函数 Accepter::newconnection()
    std::function<void()> closecallback_;   // fd_关闭(断开)的回调函数，将调用Connection::closecallback()
    std::function<void()> errorcallback_;   // fd_错误的回调函数， 将调用Connection::errorcallback() 
    std::function<void()> writecallback_;   // fd_写事件的回调函数, 将回调Connection::writecallback();
};