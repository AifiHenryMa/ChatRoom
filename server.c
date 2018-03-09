/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   server.c
 * Author: Henry Ma
 *
 * Created on March 8, 2018, 12:45 AM
 */

#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define MAXDATASIZE 256

#define SERVERPORT 4444 /*服务器监听端口*/
#define BACKLOG 1       /*最大同时连接请求数*/
#define STDIN 0         /*标准输入文件描述符*/

int main(int argc, char *argv[]) {

    FILE *fp; //定义文件类型指针
    int sockfd, client_fd; //监听sockfd，数据传输sockefd
    int sin_size;
    struct sockaddr_in my_addr, remote_addr; // 本机地址信息，客户端地址信息
    char message_buff[256]; /*用于聊天的缓冲区*/

    char name_buff[256]; //用于输入用户名的缓冲区
    char send_str[256]; //最多发出去的字符数不能超过256
    int recvbytes;
    fd_set rfd_set, wfd_set, efd_set; //被select监听的读，写，异常处理的文件描述符集合
    struct timeval timeout; // 本次select的超时结束时间
    int ret; // 与client连接的结果
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { //错误检测
        perror("socket 失败");
        exit(1);
    }

    /*填充sockaddr结构*/
    bzero(&my_addr, sizeof (struct sockaddr_in));
    my_addr.sin_family = AF_INET; // 地址族
    my_addr.sin_port = htons(SERVERPORT); // htons函数将一个32位数从主机字节顺序转换成网络字节顺序
    inet_aton("127.0.0.1", &my_addr.sin_addr);

    if (bind(sockfd, (struct sockaddr*) &my_addr, sizeof (struct sockaddr)) == -1) { // 错误检测
        perror("bind 失败");
        exit(1);
    }
    
    if(listen(sockfd,BACKLOG) == -1){ //错误检测
        perror("listen 失败");
        exit(1);
    }

    sin_size = sizeof (struct sockaddr_in);
    if ((client_fd = accept(sockfd, (struct sockaddr *)&remote_addr, &sin_size)) == -1) { // 错误检测
        perror("accept 失败");
        exit(1);
    }

    fcntl(client_fd, F_SETFD, O_NONBLOCK); // 服务器设置为非阻塞
    recvbytes = recv(client_fd, name_buff, MAXDATASIZE, 0);
    /*接收从客户端传来的用户名*/
    name_buff[recvbytes] = '\0';
    fflush(stdout); //fflush(stdout)刷新标准输出缓冲区，把输出缓冲区里的东西打印到标准输出设备上
    /*强制立即内容*/
    if ((fp = fopen("name.txt", "a+")) == NULL) {
        printf("can not open file,exit...\n");
        return -1;
    }
    fprintf(fp, "%s\n", name_buff);
    /*将用户名写入name.txt中*/
    while (1) {
        FD_ZERO(&rfd_set); // 将select()监视的读的文件描述符集合清除
        FD_ZERO(&wfd_set); // 将select()监视的写的文件描述符集合清除
        FD_ZERO(&efd_set); // 将select()监视的异常的文件描述符集合清除

        /*将标准输入文件描述符加到select()监视的读的文件描述符集合中*/
        FD_SET(STDIN, &rfd_set);

        /*将新建的描述符加到select()监视的读的文件描述符集合中*/
        FD_SET(client_fd, &rfd_set);

        /*将新建的描述符加到select()监视的写的文件描述符集合中*/
        FD_SET(client_fd, &wfd_set);

        /*将新建的描述符加到select()监视的异常的文件描述符集合中*/
        FD_SET(client_fd, &efd_set);

        timeout.tv_sec = 10; //select 在被监视的窗口等待的秒数
        timeout.tv_usec = 0; //select 在被监视的窗口等待的微秒数

        ret = select(client_fd + 1, &rfd_set, &wfd_set, &efd_set, &timeout);
        if (ret == 0) {
            //printf("select 超时\n");
            continue;
        }
        if (ret < 0) {
            perror("select 失败");
            exit(1);
        }
        /*判断标准输入文件描述符是否还在给定的描述符集rfd_set中,也就说判断是否有数据从标准输入进来*/
        if (FD_ISSET(STDIN, &rfd_set)) {
            fgets(send_str, 256, stdin); //从标准输入中取出要发送的内容
            send_str[strlen(send_str) - 1] = '\0';
            if (strncmp("quit", send_str, 4) == 0) {// 退出程序
                //printf("deubg1\n");
                close(client_fd);
                close(sockfd); //关闭套接字
                exit(0);
            }
            send(client_fd, send_str, strlen(send_str), 0);
        }

        /*判断新建的描述符是否还在给定的描述符集wfd_set中,也就说判断是否有数据从客户端发来*/
        if (FD_ISSET(client_fd, &rfd_set)) {
            recvbytes = recv(client_fd, message_buff, MAXDATASIZE, 0); //接收从客户端发来的聊天消息
            if (recvbytes == 0) {
                //printf("debug2\n");
                close(client_fd);
                close(sockfd); //关闭套接字
                exit(0);
            }
            message_buff[recvbytes] = '\0';
            printf("%s:%s\n", name_buff, message_buff);
            printf("Server: ");
            fflush(stdout);
        }

        /*判断新建的描述符是否还在给定的描述符集efd_set中,也就说判断是否有异常产生*/
        if (FD_ISSET(client_fd, &efd_set)) {
            //printf("debug3\n");
            close(client_fd);
            exit(1);
        }
    }
    return 1;
}
