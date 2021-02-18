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

// 用来处理请求
void *accept_request(void* client);

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

void *accept_request(void *client)
{
	int client_sock = *((int*)client);// 已连接的套接字

	int numchars; // 用来记录接收到的字符数
	int buf[1024]; // 表示接收缓存

	char method[255]; // 用来保存http请求头部方法(GET/POST)
	char url[255]; // 用来保存http请求的资源url
	char path[512]; // 用来保存资源路径

	// 用来记录请求是否需要执行cgi脚本，默认不执行
	// 当请求为POST请求或者GET请求中有问号的时候，则需要执行cgi脚本
	int cgi = 0;
	char *query_string = NULL; // 用来记录要查询的字符串
	
	// 先读取一行http请求
	numchars = get_line(client_sock, buf, sizeof(buf));	
	int i = 0,j = 0; // i用来遍历buf，j用来遍历method、url等

	// 保存请求中的方法
	// http请求第一行为：方法 (空格) url(空格) 主机
	while(!isspace(buf[i]) && j < (sizeof(method) - 1)){
		method[j++] = buf[i++];
	}
	method[j] = '\0';
	
	// 如果请求的方法既不是GET也不是POST，返回错误给客户端
	// TinyHttp只支持GET和POST
	if(strcmp(method, "GET") && strcmp(method, "POST")){
		unimplemented(client_sock);
		return ;
	}
	
	//跳过空格
	while(!isspace(buf[i]))
		i++;
	
	// 保存请求行中的url
	j = 0;
	while(!isspace(buf[i]) && j < (sizeof(url) - 1) && i < numchars){
		url[j++] = buf[i++];
	}
	url[j] = '\0';
	
	// 如果是POST方法，则需要执行cgi脚本
	if(strcmp(method, "POST") == 0)
		cgi = 1;
	
	// 如果是GET方法，并且请求中有问号，也需要执行cgi脚本
	if(strcmp(method, "GET") == 0)
	{
		query_string = url;
		while(*query_string != '\0'){
			if(*query_string == '?'){
				cgi = 1;
				*query_string = '\0'; //问号前面的是url,问号后面的是待查询的内容
				query_string++;
				break;
			}
			else
				query_string++;
		}
	}
	
	sprintf(path, "etc%s", url);
	// 如果请求的路径是个目录，则添加一个默认页面
	if(path[strlen(path)-1] == '/')
		strcat(path, "index.html");
	
	struct stat st;
	// 如果请求的页面不存在，则读取完剩下的请求报文并丢掉，然后返回not_found错误信息给客户端
	if(stat(path, &st) == -1){
		while(numchars > 0 && strcmp(buf, "\n"))
			numchars = get_line(client_sock, buf, sizeof(buf));
		not_found(client_sock);
	}
	else{
		// 如果待请求的文件的所有者或用户组具有可执行权限、或其他用户具有可读权限，则说明是个cgi文件
		if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
			cgi = 1;
		if(!cgi)
			server_file(client_sock, path); // 不需要执行cgi，直接返回页面给客户端
		else
			execute_cgi(client_sock, path, method, query_string); // 需要执行cgi则执行cgi脚本
	}
	close(client_sock);
}	
	










}
