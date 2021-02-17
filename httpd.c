#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define SERVER_STRING "Server: tinyhttpd/1.0.0\r\n"
#define STDIN 0
#define STDOUT 1
#define STDERR 2

// 用来初始化服务器端socket
int startup();

// 用来输出错误情况
void error_die(const char *str);

int main()
{
	int server_sock, client_sock;
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;
	pthread_t newthread;
	
	// 初始化服务器端socket
	server_sock = startup();
	printf("httpd start...\n");
	
	while(1)
	{
		// 建立一个新的客户端连接
		client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
		if(client_sock == -1)
			error_die("accept");
		// 每来一个客户端连接，就创建一个线程来处理请求
		if(pthread_create(&newthread, NULL, accept_request, (void*)&client_sock) != 0)
			error_die("pthread_create");
	}
	close(serv_sock);
	return 0;
}

int startup()
{
	struct sockaddr_in serv_addr;
	int serv_sock;
	int on = 1;
	
	int port = 12345; // 端口
	char *ip = "172.22.29.7"; // ip
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);// 创建服务器端套接字
	if(serv_sock == -1)
		error_die("socket");
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htnos(port);

	// 复用套接字的端口	
	if((setsockopt(serv_addr, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		error_die("setsockopt failed");
	if(bind(serv_sock, (strcut sockaddr*)&serv_addr, sizeof(serv_addr) == -1)
		error_die("bind");
	if(listen(serv_sock, 5) < 0)
		error_dir("listen");
	return serv_sock;
}

void error_die(const char *str)
{
	perror(str);
	exit(1);
}
