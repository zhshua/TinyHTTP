#include "log.h"

// 日志级别, 0为info级, 1为debug级, 2为warn级, 3为error级
char log_grade[][15] = { " [info]: ", " [debug]: ", " [warn]: ", " [error]: "};

// 初始化日志线程
void init_log_thread(log_thread_t *log_thread)
{
	// 初始化互斥锁
	if(pthread_mutex_init(&(log_thread->lock), NULL) != 0)
	{
		perror("fail to init lock\n");
		exit(1);
	}
	
	// 初始化条件变量
	if(pthread_cond_init(&(log_thread->log_not_empty), NULL) != 0)
	{
		perror("fail to init cond\n");
		exit(1);
	}
	
	// 初始化值
	log_thread->size = 0;
	log_thread->tail = log_thread->head = NULL;
}

// 加入一条日志到日志链表中
void push_log(int error, const char *str, int grade, log_thread_t *log_thread)
{
	msglog_t* msg_i = (msglog_t*)malloc(sizeof(msglog_t));
	strcpy(msg_i->msg, log_grade[grade]);
	strcat(msg_i->msg, strerror(error));
	strcat(msg_i->msg, str);
	msg_i->grade = grade;
	msg_i->next = NULL;
	
	// 多线程工作,加锁
	if(pthread_mutex_lock(&(log_thread->lock)) != 0)
	{
		perror("fail to lock\n");
		exit(1);
	}
	
	if(log_thread->size == 0)
		log_thread->tail = log_thread->head = msg_i;
	else
	{
		log_thread->tail->next = msg_i;
		log_thread->tail = msg_i;
	}
	++ log_thread->size;
	
	// 解锁
	if(pthread_mutex_unlock(&(log_thread->lock)) != 0)
	{
		perror("fail to unlock\n");
		exit(1);
	}
	
	// 发出条件变量信号
	if(pthread_cond_signal(&(log_thread->log_not_empty)) != 0)
	{
		perror("fail to signal\n");
		exit(1);
	}
}

void *log_work(void *arg)
{
	log_thread_t *log_thread = (log_thread_t*)(arg);
	char filename[512];
	char buf[2048], timebuf[256];
	msglog_t *ptr = NULL;

	// index是buf的下标, total是写入缓存的总大小
	int index = 0, total = 0;
	int fd;

	while(1)
	{
		get_time(timebuf);
		sprintf(filename, "../log/%s.log", timebuf);
		if((fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1)
		{
			perror("fail to open log file\n");
			exit(1);
		}
		
		if(pthread_mutex_lock(&(log_thread->lock)) != 0)
		{
			perror("fail to lock\n");
			exit(1);
		}
		while(log_thread->size == 0)
		{
			if(pthread_cond_wait(&(log_thread->log_not_empty), &(log_thread->lock)) != 0)
			{
				perror("fail to wait log_not_empty\n");
				exit(1);
			}
		}
		if(pthread_mutex_unlock(&(log_thread->lock)) != 0)
		{
			perror("fail to unlock\n");
			exit(1);
		}
		
		if(log_thread->size > 0)
		{
			if(total > 1048576) // 文件大小超过1M后换下一个文件写
			{
				get_time(timebuf);
				sprintf(filename, "../log/%s.log", timebuf);
				if((fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1)
				{
					perror("fail to open log file\n");      
					exit(1);
				}
				total = 0;
			}
			if(index > 10) // 缓存超过1024B后就写文件
			{
				if(write(fd, buf, index) < 0)
				{
					perror("fail to write\n");
					exit(1);
				}
				total += index;
				index = 0;
			}
			
			// 取出日志链表的头部节点, 写入缓存
			ptr = log_thread->head;
			get_time(timebuf);
			index = add_logbuf(buf, timebuf, index);
			index = add_logbuf(buf, ptr->msg, index);
			
			// 更新size
			if(pthread_mutex_lock(&(log_thread->lock)) != 0)
			{
				perror("fail to lock\n");
				exit(1);
			}

			-- log_thread->size;
			if(log_thread->size == 0)
				log_thread->tail = log_thread->head = NULL;
			else
				log_thread->head = log_thread->head->next;
			
			if(pthread_mutex_unlock(&(log_thread->lock)) != 0)
			{
				perror("fail to unlock\n");
				exit(1);
			}
	
		}			
	}
}

int add_logbuf(char *buf, const char *p, int index)
{
	const char *p_str = p;
	char *p_buf = buf + index;

	do
	{
		*p_buf = *p_str;
		p_buf++, p_str++;
		index++;
	} while(*p_buf != '\0');
	
	return index;
}

void get_time(char *buf)
{
	time_t timep;
	struct tm *ptr;
	time(&timep);
	ptr = localtime(&timep);
	sprintf(buf, "%04d-%02d-%02d-%02d-%02d-%02d", 1900 + ptr->tm_year, 1 + ptr->tm_mon, 
			ptr->tm_mday, ptr->tm_hour, ptr->tm_min, ptr->tm_sec);
}
