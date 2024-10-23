#include "Connection.h"

Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> clientsock)
    : loop_(loop)
    , clientsock_(std::move(clientsock))
    , disconnection_(false)
    , clientchannel_(new Channel(loop, clientsock_->fd()))
{
    // clientchannel_ = new Channel(loop_, clientsock->fd());
    // printf("bind Connection::onmessage thread is: %d.\n", syscall(SYS_gettid));
    clientchannel_->setreadcallback(std::bind(&Connection::onmessage, this));
    clientchannel_->setclosecallback(std::bind(&Connection::closecallback, this));
    clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback, this));
    clientchannel_->setwritecallback(std::bind(&Connection::writecallback, this));
    clientchannel_->useet();   // 边缘触发
    clientchannel_->enablereading();
}

Connection::~Connection() 
{ 
    // printf("conn已析构.\n"); 
}

int Connection::fd() const { return clientsock_->fd(); }

std::string Connection::ip() const { return clientsock_->ip(); }

uint16_t Connection::port() const { return clientsock_->port(); }

// TCP链接关闭(断开)的回调函数，供Channel回调
void Connection::closecallback()
{
    disconnection_ = true;
    clientchannel_->remove();
    closecallback_(shared_from_this());
}

// TCP链接错误的回调函数，供Channel回调
void Connection::errorcallback()
{
    disconnection_ = true;
    clientchannel_->remove();
    errorcallback_(shared_from_this());
}

// 处理写事件的回调函数,供Channel使用
void Connection::writecallback()
{
    int writen = ::send(fd(), outputbuffer_.data(), outputbuffer_.size(), 0);        // 尝试将发送缓冲区的数据全部发送出去
    if(writen > 0) outputbuffer_.erase(0, writen);                                   // 从outputbuffer_中删除已成功发送的字节数

    if(outputbuffer_.size() == 0) 
    { 
        clientchannel_->disablewriting();                  // 如果发送缓冲区没有数据了,那么数据已成功发送, 不在关注写事件
        sendcompletecallback_(shared_from_this());                       // 回调TcpServer::sendcomplete();            
    }
}       

void Connection::setclosecallback(std::function<void(spConnection)> fn) { closecallback_ = fn; }

void Connection::seterrorcallback(std::function<void(spConnection)> fn) { errorcallback_ = fn; }

void Connection::setonmessagecallback(std::function<void(spConnection, std::string&)> fn) { onmessagecallback_ = fn; }

void Connection::setsendcompletecallback(std::function<void(spConnection)> fn) { sendcompletecallback_ = fn; }

// 处理对端发过来的消息
void Connection::onmessage()
{
    char buffer[1024];
    // ET的工作模式,循环读取,直到数据都读完
    while(true)
    {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer));
        if(nread > 0)
        {
            // 接收到数据后先不发回去，放到接收缓冲区中
            inputbuffer_.append(buffer, nread);
        }
        else if(nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取
        {
            continue;
        }
        else if(nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已经读取完成
        {   
            std::string message;
            while(true)
            {
                if(inputbuffer_.pickmessage(message) == false) break;
                std::cout << "message: " << message << std::endl;
                // printf("message (eventfd=%d): %s\n", fd(), message.c_str());
                // 此时接收到一个完整的报文了，更新时间戳
                lasttime_ = Timestamp::now();
                // std::cout << "lasttime=" << lasttime_.tostring() << std::endl;

                // 上面已经从发来的报文中，解析出一条完整的消息，下面为对消息进行处理，这里只是简单的将消息拼接了一个字符串，然后原封不动的发回去，后续可以进行更复杂的业务处理
                onmessagecallback_(shared_from_this(), message);                  // 回调TcpSercer::onmessage();
            }
            break;
        }
        // 客户端已经断开 在一个管道或套接字中，read 返回 0 表示对方已经关闭了连接。
        else if(nread == 0)
        {
            // 返回0仅在文件末尾发生
            closecallback();             // 回调cpServer::closecallback();
            break;
        }
    }
}                                

void Connection::send(const char* data, size_t size)
{
    if(disconnection_ == true) 
    {
        printf("客户端链接已经断开了, send()直接返回。\n");
        return;
    }

    if(loop_->isinloopthread()) // 判断当前线程是否为IO线程
    {
        // 若当前线程是IO线程，直接执行发送数据的操作
        // printf("send()在事件循环的线程中\n");
        sendinloop(data, size);
    }
    else{
        // 如果当前线程不是IO线程，是工作线程，则把sendinloop()交给事件循环线程(IO线程)去执行
        // printf("send()不在事件循环的线程中\n");
        loop_->queueinloop(std::bind(&Connection::sendinloop, this, data, size));
    }
    
}

// 发送数据，如果当前线程时IO线程，则直接调用此函数，如果是工作线程，则将此函数传给IO线程
void Connection::sendinloop(const char* data, size_t size)
{
    outputbuffer_.appendwithsep(data, size);    // 把需要发送的数据写到发送缓冲区
    clientchannel_->enablewriting();     // 注册写事件
}

// 判断TCP链接是否超时(空闲太久)
bool Connection::timeout(time_t now, int val) { return now - lasttime_.toint() > val; }                        