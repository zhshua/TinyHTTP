#ifndef _HTTPCONNECTION_H_
#define _HTTPCONNECTION_H_
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include "../lock/lock.h"
#include "../CGImysql/sql_connection_pool.h"
class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

	// 主状态机的状态
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,	/* 当前正在分析请求行 */
        CHECK_STATE_HEADER,				/* 当前正在分析请求的头部字段 */
        CHECK_STATE_CONTENT				/* 当前正在分析请求的文本内容(仅用于POST请求) */
    };
		
	// 服务器处理HTTP请求的结果
    enum HTTP_CODE
    {
        NO_REQUEST,					/* 表示请求不完整, 需要继续读取客户数据 */
        GET_REQUEST,				/* 表示获得了一个完整的客户请求 */
        BAD_REQUEST,				/* 表示客户请求有语法错误 */
        NO_RESOURCE,				/* 表示客户请求的资源不存在 */
        FORBIDDEN_REQUEST,			/* 表示客户请求的资源没有权限访问 */
        FILE_REQUEST,				/* 表示请求的是个文件 */
        INTERNAL_ERROR,				/* 表示服务器内部错误 */
        CLOSED_CONNECTION			/* 表示客户端已经关闭连接 */
    };

	// 从状态机的状态
    enum LINE_STATUS
    {
        LINE_OK = 0,			/* 读取到一个完整的行 */
        LINE_BAD,				/* 读取到的行出错 */
        LINE_OPEN				/* 读取到的行数据还不完整 */
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);

private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;		/* 每个连接对应的epollfd */
    static int m_user_count;	/* 连接的客户端数量 */
    MYSQL *mysql;

private:
    int m_sockfd;				/* 每个连接对应的sockfd */
    sockaddr_in m_address;		/* 连接的客户端地址 */
    char m_read_buf[READ_BUFFER_SIZE];	/* 存储读取的请求报文数据 */
    int m_read_idx;			/* 缓冲区m_read_buf中存储的数据的最后一个字节的下一个位置 */
    int m_checked_idx;		/* 当前正在分析的字符在缓冲区中的位置 */
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];	/* 写缓冲区: 存储HTTP响应的报文数据 */
    int m_write_idx;	/* 写缓冲区中待发送的字节数 */
    CHECK_STATE m_check_state;	/* 主状态机当前的状态 */
    METHOD m_method;	
	/* 客户请求的目标文件的完整路径, 其内容等于doc_root + m_url, doc_root是网站的根目录 */
    char m_real_file[FILENAME_LEN];
    char *m_url;	/* 客户端请求的目标文件的文件名 */
    char *m_version;	/* HTTP协议版本 */
    char *m_host;	/* 主机名 */
    int m_content_length;	/* HTTP请求的消息体长度 */
    bool m_linger;		/* HTTP请求是否需要保持长连接 */
    char *m_file_address;	/* 客户请求的目标文件被mmap到内存中的起始位置 */
    struct stat m_file_stat;
	
	/**
	 * struct iovec
	 * {
	       void 	*iov_base;  // 内存起始地址
	       size_t 	iov_len;	// 这块内存的长度
	   }
	 */
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;	/* 剩余待发字节数 */
    int bytes_have_send;	/* 已发送字节数 */
};

#endif
