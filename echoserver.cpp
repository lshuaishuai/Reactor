#include <iostream>
#include <signal.h>

#include "EchoServer.h"

EchoServer *echoserver;

void Stop(int sig)
{
    printf("sig = %d\n", sig);
    // 调用EchoServer::Stop()停止服务
    echoserver->Stop();
    printf("echoserver已停止\n");
    delete echoserver;
    // printf("delete echoserver\n");

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

    echoserver = new EchoServer(argv[1], atoi(argv[2]), 3, 2);  // IO线程，工作线程
    echoserver->Start();

    return 0;
}