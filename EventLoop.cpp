#include "EventLoop.h"

int createtimerfd(int sec = 30)
{
    // 创建timerfd
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    struct itimerspec timeout;              // 定时时间的数据结构                                                  
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = sec;            // 定时时间 秒
    timeout.it_value.tv_nsec = 0;           // 纳秒
    timerfd_settime(tfd, 0, &timeout, 0);
    return tfd;
}

EventLoop::EventLoop(bool ismainloop, int timetvl, int timeout)
    :ep_(new Epoll)
    ,ismainloop_(ismainloop)
    ,wakeupfd_(eventfd(0, EFD_NONBLOCK))   // 非阻塞
    ,wakechannel_(new Channel(this, wakeupfd_))  // 这个Channel对象的loop_指向的就是当前的EventLoop对象即this，ep_为新线程新创建的，并且将wakeupfd_更新到了ep_中，让epoll监视
    // 那么执行这个handlewakeup的run就是在TcpServer的构造函数中bind给IO线程的run，并且在IO线程中执行，不会在工作线程中执行
    ,timetvl_(timetvl)
    ,timeout_(timeout)
    ,timerfd_(createtimerfd(timeout_))
    ,timerchannel_(new Channel(this, timerfd_))   // 与wakechannel_类似
    ,stop_(false)
{
    // printf("bind EventLoop::handlewakeup thread is: %d.\n", syscall(SYS_gettid));
    wakechannel_->setreadcallback(std::bind(&EventLoop::handlewakeup, this));        // 从事件循环线程执行bind的函数
    wakechannel_->enablereading();

    // 让epoll监听eimerfd，若是有时间就绪则待用回调函数 与wakechannel类似
    timerchannel_->setreadcallback(std::bind(&EventLoop::handletimer, this));
    timerchannel_->enablereading();
}

EventLoop::~EventLoop() { /*delete ep_;*/ }

// 该函数是一直在执行的，执行完一个回调函数，若是epoll监听到有新的事件就绪，还会继续处理(执行回调)
void EventLoop::run()
{
    // printf("EventLoop::run() thread is %d.\n", syscall(SYS_gettid));
    threadid_ = syscall(SYS_gettid);     // 获取事件循环所在线程的id 不能构造函数中
    while(!stop_)
    {
        // 若是没有客户端链接的话主线程也会阻塞在这里
        // 每个线程都会创建一个Epoll，所以epoll句柄都不一样
        // 若是IO线程池中一直无任务，则子线程会阻塞在这个循环
        // 因为eventfd也已经被epoll监视，若是eventfd有数据就绪的话，也会执行下面的handleevent操作；然后因为eventfd的readcallback绑定的是handlewakeup函数，所以执行的是
        std::vector<Channel*> channels = ep_->loop(10*1000);  
        if(channels.size() == 0) epolltimecallback_(this);
        else
        {
            // 有事件的fd就绪了
            // 把就绪的事件都要处理完
            for(auto& ch : channels)
            {
                ch->handleevent();
            }
        }
    }
}

// 停止事件循环
void EventLoop::stop()
{
    stop_ = true;
    // 由于事件循环阻塞在loop中，当设置stop_为true时，还要立即唤醒epoll_wait返回
    wakeup();  // 唤醒事件循环，如果没有这行代码，事件循环将在下次闹钟响时或epoll_wait超时时才会停下来
}                              

std::unique_ptr<Epoll> EventLoop::ep() { return move(ep_); }

void EventLoop::updatechannel(Channel *ch) { ep_->updatechannel(ch); }

void EventLoop::setepolltimecallback(std::function<void(EventLoop*)> fn) { epolltimecallback_ = fn; }

// 从红黑树上删除Channel
void EventLoop::removechannel(Channel* ch) { ep_->removechannel(ch); } 

// 判断当前线程是否为事件循环线程
bool EventLoop::isinloopthread() { return threadid_ == syscall(SYS_gettid); }                  

// 把任务添加到队列中
void EventLoop::queueinloop(std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> gd(mutex_);     // 给任务队列加锁
        taskqueue_.push(fn);
    }

    // 唤醒事件循环 
    wakeup();
}

// 唤醒  
void EventLoop::wakeup()
{
    // 若有工作线程 还是在工作线程中进行的
    // printf("thread is: %d.\n", syscall(SYS_gettid));
    uint64_t val = 1;
    // 只要这里往fd中写入数据，epoll就会监视到该fd有事件就绪，epoll_wait返回成功，且在构造函数参数列表将其设置为读事件，并且回调EventLoop::handlewakeup()
    write(wakeupfd_, &val, sizeof(val));
}                                   

// 事件循环线程被eventfd唤醒后执行的函数
void EventLoop::handlewakeup()
{
    // 无论是否有工作线程已经被移到IO/事件循环线程中了
    uint64_t val;
    read(wakeupfd_, &val, sizeof(val));        // 从eventfd中读取出数据，如果不读取eventfd的读事件会一直触发

    std::function<void()> fn;
    std::lock_guard<std::mutex> gd(mutex_);    // 给任务队列加锁

    while(taskqueue_.size() > 0)
    {
        fn = std::move(taskqueue_.front());
        taskqueue_.pop();
        fn();                                  // 执行任务  从而在IO线程中执行Connection::sendinloop函数
    } 
}     

// 闹钟响时的执行的函数
void EventLoop::handletimer()
{
    // 闹钟响后重新计时
    struct itimerspec timeout;              // 定时时间的数据结构                                                  
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = timetvl_;            // 定时时间 秒
    timeout.it_value.tv_nsec = 0;           // 纳秒
    timerfd_settime(timerfd_, 0, &timeout, 0);

    if(ismainloop_)
    {
        // printf("主事件循环闹钟时间到了。 thread is %d.\n", syscall(SYS_gettid));
    }
    else 
    {
        // printf("从事件循环闹钟时间到了。 thread is %d.\n", syscall(SYS_gettid));
        time_t now = time(0);
        for(auto &aa : conns_)
        {
            if(aa.second->timeout(now, timeout_))
            {
                {
                    std::lock_guard<std::mutex> gd(mmutex_);
                    conns_.erase(aa.first);    // 从事件循环map中删除超时的connection链接
                }
                timercallback_(aa.first);  // 从TcpServer的map中删除超时的conn
            }
        }
    }
}

// 把Connecttion对象保存在conns_中
void EventLoop::newconnection(spConnection conn) 
{ 
    std::lock_guard<std::mutex> gd(mmutex_);
    conns_[conn->fd()] = conn; 
}       

// 设置为TcpServer::removeconn()
void EventLoop::settimercallback(std::function<void(int)> fn) { timercallback_ = fn; }   
