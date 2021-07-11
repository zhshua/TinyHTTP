#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include <list>
#include "../lock/lock.h"
using namespace std;

class connection_pool{
public:
    MYSQL *GetConnection();     // 获取连接
    bool ReleaseConnection(MYSQL *conn);        // 释放连接
    int GetFreeConn();  // 获取空闲连接数
    void DestroyPool(); // 销毁所有连接
    void init(string url, string User, string PassWord, string DataBaseName, int Port, unsigned int MaxConn);

    static connection_pool *GetInstance();
private:
    connection_pool(){
        this->CurConn = 0;
        this->FreeConn = 0;
    }
    ~connection_pool(){
        DestroyPool();
    }
private:
    unsigned int MaxConn;   // 最大连接数
    unsigned int CurConn;   // 当前连接数
    unsigned int FreeConn;  // 空闲连接数

private:
    locker mutex;
    sem reserve;
    list<MYSQL *> connList;

private:
    string ip;     // 主机地址
    string Port;
    string User;
    string PassWord;
    string DateBase;
};

class connectionRAII{

public:
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};

#endif