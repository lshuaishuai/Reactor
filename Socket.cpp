#include "Socket.h"

int createnonblocking()
{
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP); // SOCK_NONBLOCK,将申请的fd设置为非阻塞
    if(listenfd < 0)
    {
        printf("%s: %s: %d listen socket create error: %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return listenfd;
}

// 传入一个已经准备好的fd
Socket::Socket(int fd)
    :fd_(fd)
{}

// 关闭fd
Socket::~Socket() { ::close(fd_); }

// 返回fd成员
int Socket::fd() { return fd_; }

std::string Socket::ip() const { return ip_; }

uint16_t Socket::port() const { return port_; }

void Socket::setreuseaddr(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}
void Socket::setreuseport(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));

}
void Socket::settcpnodelay(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
}
void Socket::setkeepalive(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::bind(const InetAddress& servaddr)
{
    if(::bind(fd_, servaddr.addr(), sizeof(sockaddr)) < 0)
    {
        perror("bind() failed.\n");
        exit(-1);
    }
    // 设置了监听的ip和port
    setipport(servaddr.ip(), servaddr.port());
}

void Socket::listen(int nn)
{
    if(::listen(fd_, nn) != 0)
    {
        perror("listen() failed.\n");
        exit(-1);
    }
}

int Socket::accept(InetAddress& clientaddr)
{
    struct sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(fd_, (sockaddr*)&peeraddr, &len, SOCK_NONBLOCK);
    clientaddr.setaddr(peeraddr);

    return clientfd;
}

// 设置ip和port
void Socket::setipport(const std::string &ip, uint16_t port) 
{
    ip_ = ip;
    port_ = port;
} 
