#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cout << "usage: ./tcpepoll ip port" << std::endl;
        std::cout << "example:./tcpepoll 42.193.110.107" << std::endl;
        std::cout << std::endl;
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket() failed.\n");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));

    if(connect(sockfd, (sockaddr*)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("connect(%s: %s) failed.\n", argv[1], argv[2]);
        close(sockfd);
        return -1;
    }

    printf("connect OK.\n");
    printf("start time: %d\n", time(0));
    for(int ii = 0; ii < 100000; ii++)
    {
        memset(buf, 0, sizeof(buf));
        // std::cout << "please input: ";
        // std::cin >> buf;
        sprintf(buf, "这是第%d个超级女生。", ii);

        char tmpbuf[1024];                // 临时的buffer 报文头部+报文内容
        memset(tmpbuf, 0, sizeof(tmpbuf));   
        int len = strlen(buf);            // 计算报文的大小
        memcpy(tmpbuf, &len, 4);          // 拼接报头内容  将len中的4字节内容拷贝到tmpbuf中
        memcpy(tmpbuf+4, buf, len);       // 拼接报文内容
        
        send(sockfd, tmpbuf, len + 4, 0);

        recv(sockfd, &len, 4, 0);            // 先读取4字节的报文头部
        
        memset(buf, 0, sizeof(buf));
        recv(sockfd, buf, len, 0);        // 读取报文内容
        // printf("recv: %s\n", buf);
    }
    printf("end time: %d\n", time(0));
    return 0;
}