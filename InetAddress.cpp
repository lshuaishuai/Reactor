#include "InetAddress.h"

InetAddress::InetAddress(){}


// 监听的用这个构造函数
InetAddress::InetAddress(const std::string ip, uint16_t port)
{
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
}

// 客户端来的链接用这个 
InetAddress::InetAddress(const sockaddr_in addr)
    :addr_(addr)
{}

InetAddress::~InetAddress() {}

// 返回字符串表示的地址
const char * InetAddress::ip() const { return inet_ntoa(addr_.sin_addr); }

// 返回整数表示的端口号
uint16_t InetAddress::port() const { return ntohs(addr_.sin_port); }

// 返回addr_成员的地址，转换为sockaddr类型
const sockaddr* InetAddress::addr() const { return (sockaddr*)&addr_; }

void InetAddress::setaddr(const sockaddr_in& addr) { addr_ = addr; }