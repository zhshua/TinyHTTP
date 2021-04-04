#ifndef _HANDLE_REQUEST_H_
#define _HANDLE_REQUEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include "epoll.h"

// 200 OK, 且为非keep-alive报头
#define HTTP_200_NONKEEP "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n"

// 200 OK, 且为keep-alive报头
#define HTTP_200_KEEP "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\n\
    Keep-Alive: timeout=30\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n"

// 405 且为非keep-alive报文
#define HTTP_405_KEEP "HTTP/1.1 405 Request not allow\r\nConnection: keep-alive\r\n\
    Server: dblank/1.0.0\r\nContent-Type: text/html\r\nContent-Length : 76\r\n\r\n"

// 405 且为close报头
#define HTTP_405_NONKEEP "HTTP/1.1 405 Request not allow\r\nConnection: close\r\n\
    Server: dblank/1.0.0\r\nContent-Type: text/html\r\nContent-Length : 76\r\n\r\n"

// 任务线程的入口, 每个线程不断的执行epoll_wait，根据触发的任务调用对应的函数
void *wait_task(void *arg);

// 处理405响应
void write_405(http_request_t *http_ptr);

// 处理200响应
void write_200(http_request_t *http_ptr, char *filename);

// 从请求的报头中提取url和方法,并返回keep-alive的类型
int get_url_method(char *header, char *url, char *method, size_t size);

// 接受一个http请求
void recv_http(http_request_t *http_ptr, int epfd);

// 发送一个http回复
int send_http(http_request_t *http_ptr, int epfd);

// 检查是否为keep-alive,并做相应的处理
void check_alive(http_request_t *http_ptr, int epfd);


#endif

