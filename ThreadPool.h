#pragma once

#include <iostream>
#include <sys/syscall.h>
#include <queue>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool
{
public:
    ThreadPool(size_t threadnum, std::string threadtype);                   // 在构造函数中将启动threadnum个线程
    void addtask(std::function<void()> task);       // 把任务添加到队列中
    ~ThreadPool();                                  // 在析构函数中将停止线程
    int size();
    void stop();                                    // 停止线程

private:
    std::vector<std::thread> threads_;              // 线程池中的线程
    std::queue<std::function<void()>> taskqueue_;   // 任务队列
    std::mutex mutex_;                              // 任务队列同步的互斥锁
    std::condition_variable condition_;             // 任务队列同步的条件变量
    std::atomic_bool stop_;                         // 在析构函数中，把stop_设置为true，全部的线程退出
    std::string threadtype_;                        // 线程种类  "IO", "WORKS"
};