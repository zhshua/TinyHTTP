#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define LOG_DIR "../log/"

typedef struct msglog
{
	char msg[128]; // 日志信息
	int grade; // 日志分级
	struct msglog* next; // 下一日志节点
} msglog_t;

typedef struct log_thread
{
	msglog_t *head; // 日志链表的头指针
	msglog_t *tail; // 日志链表的尾指针
	volatile int size; // 日志链表的大小
	pthread_mutex_t lock; // 互斥锁，避免访问共享资源冲突
	pthread_cond_t log_not_empty; // 条件变量，和互斥锁配合使用，避免程序一直加锁阻塞
} log_thread_t;

// 初始化日志线程
void init_log_thread(log_thread_t *log_thread);

// 日志线程工作入口
void *log_work(void *arg);

// 将日志信息推送到日志链表
void push_log(int error, const char *str, int grade, log_thread_t *log_thread);

// 获得当前系统时间
void get_time(const char *ptr);

// 将日志信息添加到日志缓冲区，并返回复制后的下标
int add_logbuf(char *buf, const char *p, int index);

#endif
