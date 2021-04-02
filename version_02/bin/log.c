#include "log.h"

// 初始化日志线程
void init_log_thread(log_thread_t *log_thread)
{
	// 初始化互斥锁
	if(pthread_mutex_init(&log_thread->lock, NULL) != 0)
	{
		perror("fail to init lock\n");
		exit(1);
	}
	
	// 初始化条件变量
	if(pthread_cond_init(&log_thread->log_not_empty, NULL) != 0)
	{
		perror("fail to init cond\n");
		exit(1);
	}
	
	// 初始化值
	log_thread->size = 0;
	log_thread->tail = log->thread->head = NULL;
}

