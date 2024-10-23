#pragma once 

#include <functional>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <memory>
#include <queue>
#include <mutex>
#include <sys/timerfd.h>
#include <time.h>
#include <map>
#include <atomic>

#include "Epoll.h"
#include "Channel.h"
#include "Timestamp.h"
#include "Connection.h"

class Epoll;
class Channel;
class Connection;

using spConnection = std::shared_ptr<Connection>;

// 事件循环类
class EventLoop
{
public:
    EventLoop(bool ismainloop, int timetvl_ = 30, int timeout = 20);
    ~EventLoop();

    void run();                                  // 运行事件循环
    void stop();                                 // 停止事件循环
    std::atomic_bool stop_;                      // 初始值为false 如果设置为true，你表示停止事件循环

    std::unique_ptr<Epoll> ep();                 // 这个函数可能有问题
    void updatechannel(Channel *ch);
    void setepolltimecallback(std::function<void(EventLoop*)>);
    void removechannel(Channel* ch);             // 从红黑树上删除Channel
    bool isinloopthread();                       // 判断当前线程是否为事件循环线程
    void queueinloop(std::function<void()> fn);  // 把任务添加到队列中
    void wakeup();                               // 唤醒      
    void handlewakeup();                         // 事件循环线程被eventfd唤醒后执行的函数
    void handletimer();                          // 闹钟响时的执行的函数

    void newconnection(spConnection conn);       // 把Connecttion对象保存在conns_中
    void settimercallback(std::function<void(int)>);  // 设置为TcpServer::removeconn()

private:
    int timetvl_;                                   // 闹钟响的时间间隔，单位：秒
    int timeout_;                                   // Connection对象超时的事件，单位：秒    
    std::unique_ptr<Epoll> ep_;
    std::function<void(EventLoop*)> epolltimecallback_;

    pid_t threadid_;                                // 事件循环所在线程的id  
    std::queue<std::function<void()>> taskqueue_;   // 事件循环线程被eventfd唤醒后执行的任务队列
    std::mutex mutex_;                              // 任务队列同步的互斥锁
    int wakeupfd_;                                  // 用于唤醒事件循环的eventfd
    std::unique_ptr<Channel> wakechannel_;          // eventfd的Channel
    int timerfd_;                                   // 定时器fd
    std::unique_ptr<Channel> timerchannel_;         // 定时器的Channel

    bool ismainloop_;                               // true表示主事件循环， false表示从事件循环
    
    std::mutex mmutex_;                             // 保护conns_的互斥锁
    std::map<int, spConnection> conns_;             // 存放运行在该事件循环上的全部Connection对象
    std::function<void(int)> timercallback_;        // 删除TcpServer中超时的Connection对象，将被设置为TcpServer::removeconn()

    // 清理空闲的connection对象步骤
    // 1.在事件循环中增加map<int, spConnection> conns_容器，存放运行在该事件循环上的全部Connection对象
    // 2.如果闹钟时间到了，遍历conns_，判断每个Connection对象是否超时
    // 3.如果超时了，从conns_中删除Connection对象
    // 4.还需要从TcpServer.conns_中删除Connection对象
    // 5.TcpServer和EventLoop的map容器需要加锁
    // 6.闹钟时间间隔和超时时间参数化
};