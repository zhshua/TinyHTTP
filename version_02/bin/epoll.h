#ifndef _EPOLL_H_
#define _EPOLL_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>

typedef struct http_request
{
	int fd; // socket描述符
	int alive; // 是否为长连接
	char method[32]; // 请求的方法
	char url[128]; // 请求的url
	char ip_port[128]; // 请求的源ip、port
}http_request_t;

// 将一个事件加入到epoll中
// epfd代表epoll的描述符, ptr是携带的结构体, status是事件的状态
void epoll_add(int epfd, http_request_t *ptr, int status);

// 将一个事件从epoll中删除
void epoll_del(int epfd, http_request_t *ptr, int status);

// 修改事件的状态
void epoll_mod(int epfd, http_request_t *ptr, int status);

// 设置套接字为非阻塞
void set_sock_nonblock(int sockfd);

#endif
