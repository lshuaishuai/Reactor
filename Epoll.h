#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <string>
#include <iostream>

#include <vector>

#include "Channel.h"

class Channel;

class Epoll
{
public:
    Epoll();
    ~Epoll();
    std::vector<Channel*> loop(int timeout=-1);    // 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返回
    int epollfd();
    void updatechannel(Channel* ch);
    void removechannel(Channel* ch);
private:
    const static int MaxEvents_ = 100; // epoll_wait()返回事件数组的大小
    int epollfd_ = -1;                 // epollfd句柄 
    epoll_event events_[MaxEvents_];    // 存放poll_wait()返回事件的数组，在构造函数中分配内存
};