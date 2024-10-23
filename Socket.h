#pragma once 

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "InetAddress.h"

// 创建listenfd
int createnonblocking();

class Socket
{
private:
    const int fd_;      // Socket持有的fd，在构造函数中传进来
    std::string ip_;    // 如果是listenfd，存放服务端监听的ip，如果是客户端连接的fd，存放对端的ip
    uint16_t port_;     // 如果是listenfd，存放服务端监听的port，如果是客户端连接的fd，存放外部端口
 
public:
    Socket(int fd);  // 传入一个已经准备好的fd
    ~Socket();       // 关闭fd

    int fd();        // 返回fd成员
    std::string ip() const;
    uint16_t port() const;
    void setipport(const std::string &ip, uint16_t port);  // 设置ip和port

    void setreuseaddr(bool on);
    void setreuseport(bool on);
    void settcpnodelay(bool on);
    void setkeepalive(bool on);

    void bind(const InetAddress& servaddr);
    void listen(int nn=128);
    int accept(InetAddress& clientaddr);
};