#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#define default_port 9190
#define default_listen_num 20
#define default_thread_num 4
#define default_file_dir "../doc"
#define default_event_list 2021
#define CONF_PATH "../etc/httpd.conf"

struct conf_http_t
{
	int port; // 监听的端口号
	int listen_num; // 监听的套接字数量最大值
	int thread_num; // 线程数量
	char doc[50]; // 资源存访的路径
	int event_list; // event事件数量
};

// 读取配置信息
void init_conf (struct conf_http_t *);

// 配置信息读取失败时读取默认信息
void init_default_conf (struct conf_http_t *);

#endif
