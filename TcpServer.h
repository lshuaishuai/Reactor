#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "EventLoop.h"
#include "Epoll.h"
#include "Socket.h"
#include "Channel.h"
#include "Accepter.h"
#include "Connection.h"
#include "ThreadPool.h"

class TcpServer
{
public:
    TcpServer(const std::string &ip, const uint16_t port, int threadnum = 3);
    ~TcpServer();

    void start();
    void stop();
    void newconnection(std::unique_ptr<Socket> clientsock);           // 使用回调函数的方法让Connection对象和Accepter对象处于同一级，处理新客户端连接请求
    void closeconnection(spConnection conn);                          // 关闭客户端的链接 在Connection类中回调此函数
    void errorconnection(spConnection conn);                          // 客户端的链接错误 在Connection类中回调此函数
    void onmessage(spConnection conn, std::string& message);          // 处理客户端的请求报文,在Connection中回调此函数
    void sendcomplete(spConnection connconn);                         // 数据发送完成后, 在Connection类中回调此函数
    void epolltimeout(EventLoop* loop);                               // epoll_wait()超时, 在EventLoop类中回调此函数

    void setnewconnectioncb(std::function<void(spConnection conn)> fn);
    void setcloseconnectioncb(std::function<void(spConnection conn)> fn);
    void seterrorconnectioncb(std::function<void(spConnection conn)> fn);
    void setonmessagecb(std::function<void(spConnection conn, std::string &message)> fn);
    void setsendcompletecb(std::function<void(spConnection conn)> fn);
    void settimeoutcb(std::function<void(EventLoop*)> fn);

    void removeconn(int fd);       // 删除conns_中的Connection对象，在EventLoop::handletimer()中将回调此函数
    void setremoveconnectioncb(std::function<void(int)> fn);

private:
    std::unique_ptr<EventLoop> mainloop_;                 // 主事件循环
    std::vector<std::unique_ptr<EventLoop>> subloops_;     // 从事件循环
    Accepter accepter_;                   // 一个TcpServer只有一个Accepter对象 只通过一个listenfd监听有无新的客户端连接上来
    int threadnum_;                       // 线程池的大小，即从事件循环的个数
    ThreadPool threadpool_;              // 线程池
    std::mutex mmutex_;                  // 保护conns_的互斥锁
    std::map<int, spConnection> conns_;   // 一个TcpServer对应多个Connection对象   int为fd  可以有多个客户端链接

    std::function<void(spConnection)> newconnectioncb_;                       // 用于回调EchoServer::HandleNewConnection()
    std::function<void(spConnection)> closeconnectioncb_;                     // 用于回调EchoServer::HandleClose()
    std::function<void(spConnection)> errorconnectioncb_;                     // 用于回调EchoServer::HandleError()
    std::function<void(spConnection, std::string &message)> onmessagecb_;     // 用于回调EchoServer::HandleMessage()
    std::function<void(spConnection)> sendcompletecb_;                        // 用于回调EchoServer::HandleSendComplete()
    std::function<void(EventLoop*)> timeoutcb_;                               // 用于回调EchoServer::HandleTimeOut()
    std::function<void(int)> removeconnectioncb_;                             // 用于回调上层业务类的HandleRemove()

};