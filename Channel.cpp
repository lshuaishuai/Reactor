#include "Channel.h"

Channel::Channel(EventLoop* loop, int fd)
    :fd_(fd)
    ,loop_(loop)
{}

// 在析构函数中,不要销毁ep_, 也不能关闭fd_,因为这两个不属于Channel类, Channel类只是使用它们
Channel::~Channel() {}

int Channel::fd() { return fd_; }

// 采用ET模式
void Channel::useet() { events_ = events_ | EPOLLET; }

// 让epoll_wait监视fd_的读事件
void Channel::enablereading()
{
    events_ = events_ | EPOLLIN;
    loop_->updatechannel(this);
}

// 取消读事件
void Channel::disablereading() 
{
    events_ &= ~EPOLLIN;
    loop_->updatechannel(this);
}

// 让epoll_wait监视fd_的写事件, 注册写事件
void Channel::enablewriting()          
{
    events_ = events_ | EPOLLOUT;
    loop_->updatechannel(this);
}

// 取消写事件
void Channel::disablewriting()         
{
    events_ &= ~EPOLLOUT;
    loop_->updatechannel(this);
}

// 取消全部的事件
void Channel::disableall()
{
    events_ = 0;
    loop_->updatechannel(this);
}   

// 从事件循环中删除全部的Channel
void Channel::remove()
{
    disableall();                  // 先取消全部的事件
    loop_->removechannel(this);    // 从红黑树上删除fd
}                 

// 把inepoll_设置为true
void Channel::setinepoll() { inepoll_ = true; }

// 设置revents_成员的值为参数ev
void Channel::setrevents(uint32_t ev) { revents_ = ev; } 

// 返回inepoll_
bool Channel::inepoll() { return inepoll_; }

// 返回events_的值                
uint32_t Channel::events() { return events_; }

// 返回revents_的值
uint32_t Channel::revents() { return revents_; }

// 事件处理函数 epoll_wait返回时,执行它
void Channel::handleevent()            
{
    // 对方已关闭，有些OS检测不到，可以使用EPOLLIN，recv返回0来表示这种情况
    // EPOLLHUP : 表示对应的文件描述符被挂断
    if(revents_ & EPOLLRDHUP) 
    {
        closecallback_();
    }
    // 接收缓冲区有数据可读   |EPOLLPRI带外数据
    else if(revents_ & (EPOLLIN | EPOLLPRI)) 
    {
        readcallback_();            // 该函数对象前后会bind两个函数 主线程执行Accepter::newconnection 从线程执行Connection::onmessage   
    }
    // 有数据需要写
    else if(revents_ & EPOLLOUT)
    {
        writecallback_();
    }
    // 其他事件，视为错误
    else{
        errorcallback_();
    }
}

// 设置fd_读事件的回调函数 Accepter::newconnection()
void Channel::setreadcallback(std::function<void()> fn) { readcallback_ = fn; } 

// 设置fd_关闭(断开)的回调函数，将调用Connection::closecallback()
void Channel::setclosecallback(std::function<void()> fn) { closecallback_ = fn; }

// 设置fd_错误的回调函数， 将调用Connection::errorcallback() 
void Channel::seterrorcallback(std::function<void()> fn) { errorcallback_ = fn; }

// 设置写事件的回调函数
void Channel::setwritecallback(std::function<void()> fn) { writecallback_ = fn; } 
