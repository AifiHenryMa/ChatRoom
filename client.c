/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   client.c
 * Author: Henry Ma
 *
 * Created on March 8, 2018, 9:16 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define SERVERPORT 4444 //服务器监听端口号
#define MAXDATASIZE 256 //最大同时连接请求
#define STDIN 0         //标准输入文件描述符

/*
 * 
 */
int main(int argc, char** argv) {

    int sockfd; //套接字描述符
    int recvbytes;
    char buf[MAXDATASIZE]; //用于处理输入的缓冲区
    char *str;

    char name[MAXDATASIZE]; //定义用户名
    char send_str[MAXDATASIZE]; //最多发出的字符不能超过MAXDATASIZE
    struct sockaddr_in serv_addr; //Internet 套接字地址结构
    fd_set rfd_set, wfd_set, efd_set; //被select()监视的读，写，异常处理的文件描述符集合
    struct timeval timeout; //本次select的超时结束时间
    int ret; //与server连接的结果

    /*创建一个socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { //错误检测
        perror("socket 失败");
        exit(1);
    }

    /*填充sockaddr结构*/
    bzero(&serv_addr, sizeof (struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVERPORT);
    inet_aton("127.0.0.1", &serv_addr.sin_addr);

    /*请求连接*/
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof (struct sockaddr)) == -1) { //错误检测
        perror("connect 失败");
        exit(1);
    }
    fcntl(sockfd, F_SETFD, O_NONBLOCK);

    printf("要聊天请首先输入你的名字：");
    scanf("%s", name);
    name[strlen(name)] = '\0';
    printf("%s: ", name);
    fflush(stdout);

    send(sockfd, name, strlen(name), 0); // 发送用户名到sockfd

    while (1) {
        FD_ZERO(&rfd_set); // 将select()监视的读的文件描述符集合清除
        FD_ZERO(&wfd_set); // 将select()监视的写的文件描述符集合清除
        FD_ZERO(&efd_set); // 将select()监视的异常的文件描述符集合清除

        /*将标准输入文件描述符加到select()监视的读的文件描述符集合中*/
        FD_SET(STDIN, &rfd_set);

        /*将新建的描述符加到select()监视的读的文件描述符集合中*/
        FD_SET(sockfd, &rfd_set);

        /*将新建的描述符加到select()监视的异常的文件描述符集合中*/
        FD_SET(sockfd, &efd_set);

        timeout.tv_sec = 10; //select 在被监视的窗口等待的秒数
        timeout.tv_usec = 0; //select 在被监视的窗口等待的微秒数

        ret = select(sockfd + 1, &rfd_set, &wfd_set, &efd_set, &timeout);
        if (ret == 0) {
            //printf("select 超时\n");
            continue;
        }

        if (ret < 0) {
            perror("select error: ");
            exit(1);
        }

        /*判断标准输入文件描述符是否还在给定的描述符集rfd_set中,也就说判断是否有数据从标准输入进来*/
        if (FD_ISSET(STDIN, &rfd_set)) {
            fgets(send_str, 256, stdin);
            send_str[strlen(send_str) - 1] = '\0';
            if (strncmp("quit", send_str, 4) == 0) { //退出程序
                close(sockfd);
                exit(0);
            }
            send(sockfd, send_str, strlen(send_str), 0);
        }
        /*判断新建的描述符是否还在给定的描述符集rfd_set中,也就说判断是否有数据从服务器端发来*/
        if (FD_ISSET(sockfd, &rfd_set)) {
            recvbytes = recv(sockfd, buf, MAXDATASIZE, 0);
            if (recvbytes == 0) {
                close(sockfd);
                exit(1);
            }

            buf[recvbytes] = '\0';
            printf("Server: %s\n", buf);

            printf("%s: ", name);
            fflush(stdout);
        }

        /*判断新建的描述符是否还在给定的描述符集efd_set中,也就说判断是否有异常产生*/
        if (FD_ISSET(sockfd, &efd_set)) {
            close(sockfd);
            exit(1);
        }
    }
    return (EXIT_SUCCESS);
}

