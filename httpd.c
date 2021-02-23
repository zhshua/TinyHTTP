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
int startup(int *port);

// 用来输出错误情况
void error_die(const char *str);

// 用来处理请求
void *accept_request(void* client);

// 用来处理错误的请求方法
void unimplemented(int client_sock);

// 用来处理404错误
void not_found(int client_sock);

// 这个函数用来读取一行http请求，并把结尾的'\r\n'替换成'\n'
int get_line(int sock, char *buf, int size);

// 传递请求的页面给客户端
void server_file(int sock, char *path);

// 用来添加http响应头部
void headers(int sock);

// 用来发送响应实体
void cat(int client_sock, FILE *file);

// 用来执行cgi脚本
void execute_cgi(int client_sock, const char *path, const char *method, const char *query_string);

// 处理错误的请求
void bad_request(int sock);

// 处理内部错误的情况
void cannot_execute(int sock);

int main(void)
{
	int server_sock, client_sock;
	struct sockaddr_in client_addr;

	socklen_t client_addr_size;
	pthread_t newthread;
	
	int port = 12345;
	// 初始化服务器端socket
	server_sock = startup(&port);
	printf("httpd start port: %d...\n",port);
	
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
	close(server_sock);
	return 0;
}

int startup(int *port)
{
	struct sockaddr_in serv_addr;
	int serv_sock;
	int on = 1;
	
	//int port = 12345; // 端口
	//char *ip = "172.22.29.7"; // ip
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);// 创建服务器端套接字
	if(serv_sock == -1)
		error_die("socket");
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(*port);

	// 复用套接字的端口	
	if((setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
		error_die("setsockopt failed");
	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_die("bind");
	if(*port == 0)
	{
		socklen_t serv_size = sizeof(serv_addr);
		if(getsockname(serv_sock, (struct sockaddr*)&serv_addr, &serv_size) < 0)
			error_die("getsockname");
		*port = ntohs(serv_addr.sin_port);
	}
	if(listen(serv_sock, 5) < 0)
		error_die("listen");
	return serv_sock;
}

void error_die(const char *str)
{
	perror(str);
	exit(1);
}

void *accept_request(void *client)
{
//	printf("accept_request...\n");
	int client_sock = *((int*)client);// 已连接的套接字

	size_t numchars; // 用来记录接收到的字符数
	char buf[1024]; // 表示接收缓存

	char method[255]; // 用来保存http请求头部方法(GET/POST)
	char url[255]; // 用来保存http请求的资源url
	char path[512]; // 用来保存资源路径

	// 用来记录请求是否需要执行cgi脚本，默认不执行
	// 当请求为POST请求或者GET请求中有问号的时候，则需要执行cgi脚本
	int cgi = 0;
	char *query_string = NULL; // 用来记录要查询的字符串
	
	// 先读取一行http请求
	numchars = get_line(client_sock, buf, sizeof(buf));	
	size_t i = 0,j = 0; // i用来遍历buf，j用来遍历method、url等
	
	// 保存请求中的方法
	// http请求第一行为：方法 (空格) url(空格) 主机
	while(!isspace(buf[i]) && (j < (sizeof(method) - 1))){
		method[j++] = buf[i++];
	}
	method[j] = '\0';
	//puts(method);
	// 如果请求的方法既不是GET也不是POST，返回错误给客户端
	// TinyHttp只支持GET和POST
	if(strcmp(method, "GET") && strcmp(method, "POST")){
		unimplemented(client_sock);
		return NULL;
	}
	
	//跳过空格
	while(isspace(buf[i]))
		i++;
	
	// 保存请求行中的url
	j = 0;
	while(!isspace(buf[i]) && j < (sizeof(url) - 1) && (i < numchars)){
		url[j++] = buf[i++];
	}
	url[j] = '\0';
	
	//puts(method);
	//puts(url);
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
		while(numchars > 0 && strcmp("\n", buf))
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
	return NULL;
}	
	


void unimplemented(int client_sock)
{
	char buf[1024];
	// 以下填充http响应报文
	sprintf(buf, "HTTP/1.1 501 Method Not Implemented\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, SERVER_STRING);
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");	
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client_sock, buf, strlen(buf), 0);
}


void not_found(int client_sock)
{
	char buf[1024];
	// 以下填充并发送404响应报文
	sprintf(buf, "HTTP/1.1 404 NOT FOUND\r\n");
    send(client_sock, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client_sock, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client_sock, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client_sock, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client_sock, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client_sock, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client_sock, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client_sock, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client_sock, buf, strlen(buf), 0);
}

int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	while((i < size - 1) && c != '\n')
	{
		if(recv(sock, &c, 1, 0) > 0)
		{
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
		//printf("%c", c);
	}
	
	// 把结尾的\r\n替换成\n	
	if(buf[i-1] == '\n' && buf[i-2] == '\r')
	{
		buf[i-2] = '\n';
		i--;
	}
	buf[i] = '\0';
	return i;
}

void server_file(int sock, char *path)
{
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024] = "A";
	
	//把剩下的http请求报文读取完，丢掉
	while(numchars > 0 && strcmp("\n", buf))
		numchars = get_line(sock, buf, sizeof(buf));
	
	resource = fopen(path, "r"); // 打开请求的页面
	if(resource == NULL)
		not_found(sock); // 请求的页面不存在，报错
	else
	{
		headers(sock);// 添加响应头部
		cat(sock, resource);// 添加响应实体
	}
	fclose(resource);
}

void headers(int sock)
{
	char buf[1024];
	
	strcpy(buf, "HTTP/1.1 200 OK\r\n");
    send(sock, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(sock, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(sock, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(sock, buf, strlen(buf), 0);
}

void cat(int client_sock, FILE *file)
{
	char buf[1024];
	
	fgets(buf, sizeof(buf), file);
	while(!feof(file))
	{
		send(client_sock, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), file);
	}
}

void execute_cgi(int client_sock, const char *path, const char *method, const char *query_string)
{
	
	int input[2];// 由父进程向子进程的输入管道
	int output[2];//由子进程向父进程的输出管道
	int status; //捕获进程结束信息
	
	int numchars = 1;
	char buf[1024] = "A";//接收缓存
	char c; //接收的单个字符
	int content_length = -1;// 用来保存POST请求的数据长度
	
	// 如果是GET请求，则不需要读取表单数据，把头部读取完扔掉即可
	if(strcmp(method, "GET") == 0)
	{
		while(numchars > 0 && strcmp(buf, "\n"))// numchars和buf已经在定义的时候分别初始化了
			numchars = get_line(client_sock, buf, sizeof(buf));
	}
	else if(strcmp(method, "POST") == 0) //如果是POST请求，则保存一下请求的长度方便后续读取
	{
		//printf("POST...........\n");
		numchars = get_line(client_sock, buf, sizeof(buf));//这里先读取一行字符，顺便也初始化了numchars和buf
		while(numchars > 0 && strcmp(buf, "\n"))
		{
			buf[15] = '\0';//截断一下读取的行，因为Content-Length:是15个字符
			if(strcmp(buf, "Content-Length:") == 0)
				content_length = atoi(&buf[16]);//保存POST请求的数据长度
			numchars = get_line(client_sock, buf, sizeof(buf));
		}
		//printf("%d\n", content_length);
		if(content_length == -1){
			bad_request(client_sock);
			return ;
		}
	}
	else 
	{
		// 如果后续实现了其他请求可补充
	}
	
	pid_t pid;
	if(pipe(input) < 0){
		cannot_execute(client_sock);
		return ;
	}
	if(pipe(output) < 0){
		cannot_execute(client_sock);
		return ;
	}
	if((pid = fork()) < 0){
		cannot_execute(client_sock);
		return ;
	}
	
	// 发送响应状态行
	sprintf(buf, "HTTP/1.1 200 OK\r\n");
	send(client_sock, buf, strlen(buf), 0);
	
	// 如果是子进程，则执行cgi程序
	if(pid == 0)
	{
		// 下面三个数组用来存放执行cgi时候需要的环境变量
		char meth_env[255];
		char query_env[255];
		char length_env[255];

		dup2(output[1], STDOUT); // 复制output[1]到子进程的标准输出(把子进程的标准输出由output[1]端输出进output管道)
		dup2(input[0],STDIN);	// 复制input[0]到子进程的标准输入(把input管道中的内容由input[0]输入到子进程的标准输出)
		
		//关闭不用的管道口
		close(output[0]);
		close(input[1]);
		
		// 下面写环境变量到系统中
		sprintf(meth_env, "REQUEST_METHOD=%s", method);
		putenv(meth_env);
		if(strcmp(method, "GET") == 0){
			sprintf(query_env, "QUERY_STRING=%s", query_string);
			putenv(query_env);
		}
		else
		{
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(length_env);
		}
		// 执行cgi脚本
		execl(path, path, NULL);
		exit(0);
	}
	else
	{
		// 关闭父进程不必要的管道口
		close(output[1]);
		close(input[0]);	
	
		if(strcmp(method, "POST") == 0)
		{
			// 父进程往子进程写输入
			for(int i = 0;i < content_length;++i){
				recv(client_sock, &c, 1, 0);
				write(input[1], &c, 1);
			}
		}
		while(read(output[0], &c, 1) > 0){
			send(client_sock, &c, 1, 0);
		}
		
		close(output[0]);
		close(input[1]);
		waitpid(pid, &status, 0);
	}		
}	

void bad_request(int sock)
{
	char buf[1024];

    sprintf(buf, "HTTP/1.1 400 BAD REQUEST\r\n");
    send(sock, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(sock, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(sock, buf, strlen(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(sock, buf, strlen(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(sock, buf, strlen(buf), 0);
}

void cannot_execute(int sock)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.1 500 Internal Server Error\r\n");
    send(sock, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(sock, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(sock, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(sock, buf, strlen(buf), 0);
}
