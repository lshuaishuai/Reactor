#include <iostream>
#include <signal.h>

#include "BankServer.h"

BankServer *bankserver;

void Stop(int sig)
{
    printf("sig = %d\n", sig);
    // 调用BankServer::Stop()停止服务
    bankserver->Stop();
    printf("bankserver已停止\n");
    delete bankserver;
    // printf("delete bankserver\n");

    exit(0);
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cout << "usage: ./tcpepoll ip port" << std::endl;
        std::cout << "example:./tcpepoll 42.193.110.107" << std::endl;
        std::cout << std::endl;
        return -1;
    }

    signal(SIGTERM, Stop);     // 信号15，系统kill或killall命令默认发送的信号
    signal(SIGINT, Stop);      // 信号2， 按Ctrl+C发送的信号    

    bankserver = new BankServer(argv[1], atoi(argv[2]), 3, 2);  // IO线程，工作线程
    bankserver->Start();

    return 0;
}