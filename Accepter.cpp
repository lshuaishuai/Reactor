#include "Accepter.h"

Accepter::Accepter(EventLoop* loop, const std::string &ip, const uint16_t port)
    :loop_(loop)
    ,servsock_(createnonblocking())
    ,acceptchannel_(loop_, servsock_.fd())
{
    // 创建listenfd 并创建server的Socket对象
    // Socket对象的析构函数会关闭fd_,所以需要将变量放在堆上,防止close
    // servsock_ = new Socket(createnonblocking());
    // 服务端的ip和port也构造一个对象
    InetAddress servaddr(ip, port); 
    // 端口复用
    servsock_.setreuseaddr(true);
    servsock_.setkeepalive(true);
    servsock_.setreuseport(true);
    servsock_.settcpnodelay(true);

    // 绑定 监听
    servsock_.bind(servaddr);
    servsock_.listen(128);

    // 将fd交给epoll等待
    // Epoll ep;

    // acceptchannel_ = new Channel(loop_, servsock_.fd());
    acceptchannel_.setreadcallback(std::bind(&Accepter::newconnection, this));    // 监听的fd若检测到有新的链接来也是EPOLLIN
    acceptchannel_.enablereading();
}

Accepter::~Accepter() {}


// 处理新客户端连接请求
void Accepter::newconnection()
{
    InetAddress clientaddr; // 客户端的ip和port
    std::unique_ptr<Socket> clientsock(new Socket(servsock_.accept(clientaddr)));
    clientsock->setipport(clientaddr.ip(), clientaddr.port());

    // 这里实际调用的是TcpServer::newconnection(clientsock)  是为了在TcpServer中创建Connection对象
    newconnectioncb_(std::move(clientsock));
}   

// 设置处理新客户端连接请求的回调函数
void Accepter::setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn) { newconnectioncb_ = fn; }   