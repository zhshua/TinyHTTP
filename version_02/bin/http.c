#include "http.h"
#include "config.h"
#include <netinet/tcp.h>

int main()
{
	
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
