#pragma once

#include <functional>
#include <memory>
#include <atomic>

#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"
#include "Timestamp.h"

class EventLoop;
class Channel;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class Connection: public std::enable_shared_from_this<Connection>
{
public:
    Connection(EventLoop* loop, std::unique_ptr<Socket> clientsock);
    ~Connection();

    int fd() const;        // 返回fd成员
    std::string ip() const;
    uint16_t port() const;

    void closecallback();        // TCP链接关闭(断开)的回调函数，供Channel回调
    void errorcallback();        // TCP链接错误的回调函数，供Channel回调
    void onmessage();            // 处理对端发过来的消息
    void writecallback();        // 处理写事件的回调函数,供Channel使用

    void setclosecallback(std::function<void(spConnection)> fn);
    void seterrorcallback(std::function<void(spConnection)> fn);
    void setonmessagecallback(std::function<void(spConnection, std::string&)> fn);
    void setsendcompletecallback(std::function<void(spConnection)> fn);

    void send(const char* data, size_t size);        // 发送数据，不管在任何线程中，都调用此函数
    void sendinloop(const char* data, size_t size);  // 发送数据，如果当前线程时IO线程，则直接调用此函数，如果是工作线程，则将此函数传给IO线程
    bool timeout(time_t now, int val);                        // 判断TCP链接是否超时(空闲太久)
private:
    EventLoop* loop_;            // Accepter对应的事件循环,在构造函数中传入
    std::unique_ptr<Socket> clientsock_;         // 服务端用于监听的socket,在构造函数中创建
    std::unique_ptr<Channel> clientchannel_;     // Accepter对应的channel, 在构造函数中创建
    Buffer inputbuffer_;         // 接收缓冲区
    Buffer outputbuffer_;        // 发送缓冲区
    std::atomic_bool disconnection_;             // 客户端链接是否已断开，断开为true  IO线程修改其值 工作线程判断改值
    Timestamp lasttime_;         // 时间戳，创建Connection对象时为当前时间，没接收到一个报文，把时间戳更新为当前时间   

    std::function<void(spConnection)> closecallback_;   // 关闭fd_的回调函数，将回调TcpServer::closeconnection()
    std::function<void(spConnection)> errorcallback_;
    std::function<void(spConnection, std::string&)> onmessagecallback_;     // 处理报文的回调函数
    std::function<void(spConnection)> sendcompletecallback_; 
};