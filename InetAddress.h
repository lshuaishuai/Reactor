#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string>

// scoket的地址协议类
class InetAddress
{
public:
    InetAddress();
    // 监听的用这个构造函数
    InetAddress(const std::string ip, uint16_t port);

    // 客户端来的链接用这个 
    InetAddress(const sockaddr_in addr);

    ~InetAddress();
    const char *ip() const;        // 返回字符串表示的地址
    uint16_t port() const;         // 返回整数表示的端口号
    const sockaddr* addr() const;  // 返回addr_成员的地址，转换为sockaddr类型
    void setaddr(const sockaddr_in& addr);


private:
    struct sockaddr_in addr_; // 表示地址协议的结构体
};