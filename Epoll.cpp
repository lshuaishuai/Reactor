#include "Epoll.h"

Epoll::Epoll()
{
    epollfd_ = epoll_create(1);
    if(epollfd_ == -1)
    {
        printf("epoll_create() failed(%d)\n!", errno);
        exit(-1);
    }
}

Epoll::~Epoll() { close(epollfd_); }

std::vector<Channel*> Epoll::loop(int timeout)    
{
    std::vector<Channel*> channels;

    bzero(events_, sizeof(events_));
    int infds = epoll_wait(epollfd_, events_, MaxEvents_, timeout);
    if(infds < 0)
    {
        // EBADF: epfd不是一个有效的描述符
        // EFAULTT: 参数events指向的内存区域不可写
        // EINVAL: epfd不是一个epoll文件描述符,或者参数maxevents小于等于0
        // EINTR: 阻塞过程中被信号终端, epoll_pwait()可以避免,或者错误处理中解析error后重新epoll_wait()
        // 在Reactor中,不建议使用信号
        perror("epoll_wait() failed!");
        exit(-1);
    }
    // 超时
    if(infds == 0)
    {
        // 如果epoll_wait()超时,表示系统很空闲,返回的channels将为空
        // printf("epoll_wait() timeout.\n");
        return channels;
    }
    // printf("epoll_wait success! process id: %d\n", syscall(SYS_gettid));
    // 如果infds>0, 表示有事件发生的fd的数量
    for(int ii = 0; ii < infds; ii++)
    {
        // evs.push_back(events_[ii]);
        Channel *ch = (Channel*)events_[ii].data.ptr;
        ch->setrevents(events_[ii].events);            // 设置就绪事件
        channels.push_back(ch);
    } 
    return channels;
}

int Epoll::epollfd() { return epollfd_; };

void Epoll::updatechannel(Channel* ch)
{
    epoll_event ev;
    ev.data.ptr = ch;
    ev.events = ch->events();  // 指定事件

    // channel在树上就更新否则添加
    if(ch->inepoll())
    {
        if(epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev) == -1)
        {
            perror("epoll_ctl() failed.\n");
            exit(-1);
        }
    }
    else{
        if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev) == -1)
        {
            perror("epoll_ctl() failed.\n");
            exit(-1);
        }
        ch->setinepoll();  // 把channel的inepoll_成员设置为true
    }
}


void Epoll::removechannel(Channel* ch)
{
    if(ch->inepoll())
    {
        // printf("removechannel()\n");
        if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), 0) == -1)
        {
            perror("epoll_ctl() failed.\n");
            exit(-1);
        }
    }
}
                      
