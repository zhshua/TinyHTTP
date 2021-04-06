#include "http.h"
#include "config.h"
#include "handle_request.h"
#include "epoll.h"
#include "log.h"
#include <netinet/tcp.h>

struct conf_http_t conf;
log_thread_t log_thread;

int main()
{
	int serv_sock, client_sock;
	uint16_t serv_port;
	int *epfds = NULL;
	char client_ip[128];
	pthread_t log_id;
	
	// 初始化日志链表和配置信息
	init_log_thread(&log_thread);
	init_conf(&conf);	
	
	// 申请几个存放epoll空间的描述符
	epfds = (int *)malloc(sizeof(int) * conf.thread_num);
	if(epfds == NULL)
	{
		perror("fail to malloc epfds\n");
		exit(1);
	}
	
	// 创建几个线程
	pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * conf.thread_num);
	if(threads == NULL)
	{
		perror("fail to malloc threads\n");
		exit(1);
	}
	
	// 监听套接字
	serv_port = conf.port;
	serv_sock = startup(&serv_port, conf.listen_num);
	
	// 创建epoll空间
	for(int i = 0;i < conf.thread_num;i++)
	{
		if((epfds[i] = epoll_create1(0)) < 0)
		{
			perror("fail to epoll_create1\n");
			exit(1);
		}
	}
	
	// 创建工作线程
	for(int i = 0;i < conf.thread_num;i++)
	{
		if((pthread_create(&threads[i], NULL, wait_task, (void *)(epfds + i))) != 0)
		{
			perror("fail to create thread\n");
			exit(1);
		}
	}
	
	// 创建日志线程
	if((pthread_create(&log_id, NULL, log_work, &log_thread)) != 0)
	{
		perror("fail to create log thread\n");
		exit(1);
	}

	struct sockaddr_in client_addr;
	socklen_t client_sz = sizeof(struct sockaddr_in);

	// 主线程监听套接字, 并下发到各线程的epoll
	while(1)
	{
		for(int i = 0;i < conf.thread_num;i++)
		{
			if((client_sock = accept(serv_sock, (struct sockaddr*)&client_addr, &client_sz)) < 0)
			{
				push_log(errno, ": accept fail\n\n", 2, &log_thread);
				continue;
			}
			http_request_t *ptr = (http_request_t *)malloc(sizeof(http_request_t));
			ptr->fd = client_sock;
			ptr->alive = 0;
			epoll_add(epfds[i], ptr, EPOLLIN);
			
			//添加接入日志
			sprintf(ptr->ip_port, " ip: %s port: %d connect\n\n", inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 128), ntohs(client_addr.sin_port));
            push_log(errno, ptr->ip_port, 0, &log_thread);
		//	puts(ptr->ip_port);
		}
	}
	return 0;
}


int startup(uint16_t *server_port, int listen_num)
{
	struct sockaddr_in server_addr;
	int server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sock == -1)
	{
		perror("server_sock create fail\n");
		exit(1);
	}
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*server_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// 设置端口重用
	int on = 1;
	if( (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0 )
	{
		perror("so_reuseaddr fail\n");
	}
	
	setsockopt(server_sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));// 关闭ngale算法

	if( (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0)
	{
		perror("bind fail\n");
		exit(1);
	}
	
	if(listen(server_sock, listen_num) < 0)
	{
		perror("listen fail\n");
		exit(1);
	}
	
	return server_sock;
}
