#include "handle_request.h"
#include "log.h"
#include "epoll.h"
#include "config.h"

extern struct conf_http_t conf;
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
				}
				else
				{
					push_log(errno, ": return 200\n\n", 0, &log_thread);
				}
			}
		}
				
	}
	free(evlist);
}

// 处理405响应
void write_405(http_request_t *http_ptr)
{
	char body[128] = "<html> <h1>Hahahaha, we only accept GET because the maker is too low!</h1>\r\n";
	char header[256] = { '\0' };
	
	if(http_ptr->alive)
		strcpy(header, HTTP_405_KEEP);
	else
		strcpy(header, HTTP_405_NONKEEP);
	
	if(write(http_ptr->fd, header, strlen(header)) < 0)
	{
		push_log(errno, ": faile to write header\n\n", 2, &log_thread);
	}	
	if(write(http_ptr->fd, body, strlen(body)) < 0)
		push_log(errno, ": fail to write body\n\n", 2, &log_thread);	
}


// 处理200响应
int write_200(http_request_t *http_ptr, char *filename)
{
	struct stat st;
	char header[2048] = { '\0' };
	
	if(stat(filename, &st) == -1 || S_ISDIR(st.st_mode))
	{
		write_405(http_ptr);
		return -1;
	}
	
	if(http_ptr->alive)
		sprintf(header, HTTP_200_KEEP, st.st_size);
	else
		sprintf(header, HTTP_200_NONKEEP, st.st_size);
	
	if(write(http_ptr->fd, header, strlen(header)) < 0)
	{
		push_log(errno, ": fail to write\n\n", 2, &log_thread);
		return -1;
	}	
	
	// puts(header);	
	
	int fd = open(filename, O_RDONLY);
	if(fd == -1)
	{
		push_log(errno, ": fail to open filename\n\n", 2, &log_thread);
		return -1;
	}
	
	if(sendfile(http_ptr->fd, fd, NULL, st.st_size) < 0)
	{
		push_log(errno, ": fail to sendfile\n\n", 2, &log_thread);
		return -1;
	}
	
	close(fd);
	return 0;
}


// 从请求的报头中提取url和方法,并返回keep-alive的类型
int get_url_method(char *header, char *url, char *method, size_t size)
{
	size_t i = 0, j = 0;
	while(header[i] != ' ' && i < size && j < 32)
	{
		method[j] = header[i];
		i++, j++;
	}
	method[j] = '\0';
	
	while(header[i] == ' ')
		i++;
	
	j = 0;
	while(header[i] != ' ' && i < size && j < 128)
	{
		url[j] = header[i];
		i++, j++;
	}
	url[j] = '\0';
	//printf("size = %d\n", size);
	//printf("i = %d\n", i);
	//printf("j = %d\n", j);
	//printf("header = %s\n", header);
	//printf("url = %s\n", url);
	
	int alive = 0;
	if(strcasestr(header, "keep-alive") != NULL)
		alive = 1;
	return alive;
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
	size_t url_len, mark = 0;
	if(strcasecmp(http_ptr->method, "GET") != 0)
	{
		write_405(http_ptr);
		check_alive(http_ptr, epfd);
		return -1;
	}
	
	url_len = strlen(http_ptr->url);
	if(url_len <= 0)
	{
		write_405(http_ptr);
		check_alive(http_ptr, epfd);
		return -1;
	}
	
	char path[128] = { '\0' };
	strcpy(path, conf.doc);
	strcat(path, http_ptr->url);

//	puts(path);	
	if(path[strlen(path) - 1] == '/')
		strcat(path, "index.html");
	
	mark = write_200(http_ptr, path);
	check_alive(http_ptr, epfd);
	return mark;
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
