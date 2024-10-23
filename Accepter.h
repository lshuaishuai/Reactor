#pragma once

#include <functional>
#include <memory>

#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Connection.h"

class Accepter
{
public:
    Accepter(EventLoop* loop, const std::string &ip, const uint16_t port);
    ~Accepter();

    void newconnection();           // 处理新客户端连接请求

    void setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn);      // 设置处理新客户端连接请求的回调函数, 将指向TcpServer::newconnection()
private:
    EventLoop* loop_;          // Accepter对应的事件循环,在构造函数中传入
    Socket servsock_;         // 服务端用于监听的socket,在构造函数中创建
    Channel acceptchannel_;   // Accepter对应的channel, 在构造函数中创建
    std::function<void(std::unique_ptr<Socket>)> newconnectioncb_;   // 设置处理新客户端连接请求的回调函数 
};