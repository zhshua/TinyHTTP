#ifndef _HTTP_H_
#define _HTTP_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int startup(uint16_t *server_port, int listen_num);

#endif
