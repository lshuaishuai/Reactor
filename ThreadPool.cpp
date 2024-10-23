#include "ThreadPool.h"

// 在构造函数中将启动threadnum个线程
ThreadPool::ThreadPool(size_t threadnum, std::string threadtype)
    :stop_(false), threadtype_(threadtype)
{
    // 启动threadnum个线程，每个线程将阻塞在条件变量上
    for(int ii = 0; ii < threadnum; ii++)
    {
        // 用lambda函数创建线程
        // threads_.emplace_back([]{...}) 创建了一个线程 传入的lambda是线程要执行的函数体
        threads_.emplace_back([this]
        {
            printf("create %s thread(%d).\n", threadtype_.c_str(), syscall(SYS_gettid));                    // 显示线程id
            // std::cout << "子线程：" << std::this_thread::get_id() << std::endl;   // 显示线程的id

            while(stop_ == false)
            {
                std::function<void()> task;      // 存放出队的元素
                
                // 锁的作stop用域
                {
                    std::unique_lock<std::mutex> lock(this->mutex_);
 
                    // 等待生产者的条件变量 若条件不满足则线程被挂起且释放锁，直到有线程唤醒该线程 若一傲剑满足则执行下面的代码
                    this->condition_.wait(lock, [this]
                    {
                        return ((this->stop_==true) || (this->taskqueue_.empty() == false));
                    });

                    // 在线程池停止之前，如果队列中还有任务，执行完再退出
                    if((this->stop_ == true) && (this->taskqueue_.empty() == true)) return;

                    // 出队一个任务
                    task = std::move(this->taskqueue_.front());
                    this->taskqueue_.pop();
                }

                // printf("%s(%d) execute task.\n", threadtype_.c_str(), syscall(SYS_gettid));
                task();  // 执行任务
            }
        });
    }
}            

void ThreadPool::addtask(std::function<void()> task)
{
    {
        // 锁作用域的开始
        std::lock_guard<std::mutex> lock(mutex_);
        taskqueue_.push(task);
    }
    condition_.notify_one();     // 唤醒一个线程 队列中有任务了，消费者可以执行了
}

// 在析构函数中将停止线程
ThreadPool::~ThreadPool() { stop(); }                                  

int ThreadPool::size() { return threads_.size(); }

// 停止线程
void ThreadPool::stop()
{
    if(stop_) return;
    stop_ = true;
    condition_.notify_all();     // 唤醒全部的线程

    // 等待全部的线程执行完任务后退出
    for(std::thread &th : threads_)
        th.join();
}                                    