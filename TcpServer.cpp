#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, const uint16_t port, int threadnum)
    :threadnum_(threadnum)
    ,mainloop_(new EventLoop(true))
    ,accepter_(mainloop_.get(), ip, port)
    ,threadpool_(threadnum_, "IO")          // 一创建线程池子线程就会启动了 若任务队列无任务，线程会被条件变量挂起，等待，直到调用ThreadPool::addtask，添加任务并且唤醒进程
{
    mainloop_->setepolltimecallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1));       // 主事件循环若一直没有新的链接上来也会timeout
    accepter_.setnewconnectioncb(std::bind(&TcpServer::newconnection, this, std::placeholders::_1));

    // 创建从事件循环
    for(int ii = 0; ii < threadnum_; ii++)
    {
        subloops_.emplace_back(new EventLoop(false, 5, 10));           // 创建从事件循环，存入容器中 每个从线程都会创建一个epoll句柄
        subloops_[ii]->setepolltimecallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1)); // 设置timeout的回调函数
        subloops_[ii]->settimercallback(bind(&TcpServer::removeconn, this, std::placeholders::_1));
        // 向任务队列中添加任务，并且唤醒被挂起的线程 线程被唤醒，就会执行添加到队列中的任务 事件循环线程执行一开始执行EventLoop::run的时候，会超时，因为epoll_wait没有监听到事件就绪 直到有客户端的链接发来消失时
        threadpool_.addtask(std::bind(&EventLoop::run, subloops_[ii].get()));       // 每个线程的run函数都是一直运行的(死循环)，所以只要是epoll中监视的fd，只要有事件就绪就可以epoll_wait成功，进而执行相应的操作
    }
}

TcpServer::~TcpServer() {}

void TcpServer::start() { mainloop_->run(); }
void TcpServer::stop() 
{
    // 停止主时间循环
    mainloop_->stop();
    // printf("主事件循环已停止\n");
    // 停止从事件循环
    for(int ii = 0; ii < threadnum_; ii++)
    {
        subloops_[ii]->stop();
    }
    // printf("从事件循环已停止\n");
    // 停止IO线程 
    threadpool_.stop(); 
    // printf("IO线程已停止\n");
}


// 处理新客户端连接请求
void TcpServer::newconnection(std::unique_ptr<Socket> clientsock)
{
    // Connection *conn = new Connection(mainloop_, clientsock);  // 之后再解决delete问题
    spConnection conn(new Connection(subloops_[clientsock->fd()%threadnum_].get(), std::move(clientsock)));  // 将 connection对象分配给从事件循环，来处理收发数据  
    conn->setclosecallback(std::bind(&TcpServer::closeconnection, this, std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection, this, std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage, this, std::placeholders::_1, std::placeholders::_2));
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete, this, std::placeholders::_1));

    {   
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_[conn->fd()] = conn;                                   // 存放到map容器中

    }
    subloops_[conn->fd()%threadnum_]->newconnection(conn);  // 把conn存放到EventLoop的map容器中 
    if(newconnectioncb_) newconnectioncb_(conn);                 // 回调 链接建立完成后再回调
}           

void TcpServer::closeconnection(spConnection conn)
{ 
    if(closeconnectioncb_) closeconnectioncb_(conn);  // 先回调再关闭

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->fd());
    }
}

void TcpServer::errorconnection(spConnection conn)
{
    if(errorconnectioncb_) errorconnectioncb_(conn);
    // printf("client(eventfd=%d) error.\n", conn->fd());
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->fd());
    }
}

// 处理客户端的请求报文,在Connection中回调此函数
void TcpServer::onmessage(spConnection conn, std::string& message)
{
    if(onmessagecb_) onmessagecb_(conn, message);
}

// 数据发送完成后, 在Connection类中回调此函数
void TcpServer::sendcomplete(spConnection conn)
{
    if(sendcompletecb_) sendcompletecb_(conn);
}             

// epoll_wait()超时, 在EventLoop类中回调此函数
void TcpServer::epolltimeout(EventLoop* loop)
{
    if(timeoutcb_) timeoutcb_(loop);
}

// 删除conns_中的Connection对象，在EventLoop::handletimer()中将回调此函数
void TcpServer::removeconn(int fd) 
{ 
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(fd);
    } 
    if(removeconnectioncb_) removeconnectioncb_(fd);
}

void TcpServer::setnewconnectioncb(std::function<void(spConnection)> fn) { newconnectioncb_ = fn; }
void TcpServer::setcloseconnectioncb(std::function<void(spConnection)> fn) { closeconnectioncb_ = fn; }
void TcpServer::seterrorconnectioncb(std::function<void(spConnection)> fn) { errorconnectioncb_ = fn; }
void TcpServer::setonmessagecb(std::function<void(spConnection, std::string &message)> fn) { onmessagecb_ = fn; }
void TcpServer::setsendcompletecb(std::function<void(spConnection)> fn) { sendcompletecb_ = fn; }
void TcpServer::settimeoutcb(std::function<void(EventLoop*)> fn) { timeoutcb_ = fn; }
void TcpServer::setremoveconnectioncb(std::function<void(int)> fn) { removeconnectioncb_ = fn; }
