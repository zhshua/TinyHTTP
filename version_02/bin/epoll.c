#include "epoll.h"

void epoll_add(int epfd, http_request_t *ptr, int status)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.data.ptr = ptr;
	ev.events = status;
	set_sock_nonblock(ptr->fd);
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, ptr->fd, &ev) == -1)
	{
		perror("epoll_ctl_add fail\n");
	}
	return ;
}

void epoll_del(int epfd, http_request_t *ptr, int status)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.data.ptr = ptr;
	ev.events = status;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, ptr->fd, &ev) == -1)
	{
		perror("epoll_ctl_del fail\n");
	}
	return ;
}

void epoll_mod(int epfd, http_request_t *ptr, int status)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.data.ptr = ptr;
	ev.events = status;
	if(epoll_ctl(epfd, EPOLL_CTL_MOD, ptr->fd, &ev) == -1)
	{
		perror("epoll_ctl_mod fail\n");
	}
	return ;
}

void set_sock_nonblock(int sockfd)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if(flags == -1)
	{
		perror("fcntl get old flags fail\n");
		return;
	}

	if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		perror("fcntl set nonblock fail\n");
	}
	return ;
}
		
