#include "http_request.h"
#include "log.h"
#include "epoll.h"
#include "config.h"

extern struct http_conf_t conf;
extern log_thread_t log_thread;

// 任务线程的入口, 每个线程不断的执行epoll_wait，根据触发的任务调用对应的函数
void *wait_task(void *arg)
{
	int epfd = *(int *)arg;
	struct epoll_event *evlist = NULL; // 存储epoll返回的就绪事件列表
	evlist = ( struct epoll_event* )malloc(sizeof(struct epoll_event) * conf.event_num);
	
	if(evlist == NULL)
	{
		perror("fail to malloc evlist\n");
		exit(1);
	}
	
	int n;
	http_request_t *ptr = NULL;
	while(1)
	{
		n = epoll_wait(epfd, evlist, conf.event_num, -1);
		if(n < 0)
		{
			push_log(errno, ": epoll_wait fail\n\n", 2, &log_thread);
			continue;
		}
		
		for(int i = 0;i < n;i++)
		{
			ptr = (http_request_t *)evlist[i].data.ptr;
			if(evlist[i].events == EPOLLIN)
				recv_http(ptr, epfd);
			else if(evlist[i].events == EPOLLOUT)
			{
				if(send_http(ptr, epfd) == -1)
				{
					push_log(errno, ": return 405\n\n", 0, &log_thread);
					write_405(ptr);
				}
				else
				{
					push_log(errno, ": return 200\n\n", 0, &log_thread);
					write_200(ptr);
				}
			}
		}
				
	}
	free(evlist);
}

// 处理405响应
void write_405(http_request_t *http_ptr)
{

}

// 处理200响应
void write_200(http_request_t *http_ptr, char *filename)
{

}

// 从请求的报头中提取url和方法,并返回keep-alive的类型
int get_url_method(char *header, char *url, char *method, size_t size)
{

}

// 接受一个http请求
void recv_http(http_request_t *http_ptr, int epfd)
{
	char buf[10240];
	size_t n;
	int alive = 0;
	int sockfd = http_ptr->fd;
	
	n = recv(sockfd, buf, 10240, 0);
	if(n < 0)
	{
		push_log(errno, ": recv socket fail\n\n", 2, &log_thread);
		close(sockfd);
		http_ptr = NULL;
	}
	else if(n == 0)
	{
		push_log(errno, ": socket connect close\n\n", 0, &log_thread);
		close(sockfd);
		http_ptr = NULL;
	}
	else
	{
		alive = get_url_method(buf, http_ptr->url, http_ptr->method, n);
		http_ptr->alive = alive;
		epoll_mod(epfd, http_ptr, EPOLLOUT);
	}

}

// 发送一个http回复
int send_http(http_request_t *http_ptr, int epfd)
{

}

// 检查是否为keep-alive,并做相应的处理
void check_alive(http_request_t *http_ptr, int epfd)
{
	int alive = http_ptr->alive;
	if(alive)
	{
		epoll_mod(epfd, http_ptr, EPOLLIN);
	}
	else
	{
		close(http_ptr->fd);
		http_ptr = NULL;
	}
}
