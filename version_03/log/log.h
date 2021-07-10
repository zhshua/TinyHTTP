#ifndef _LOG_H_
#define _LOG_H_

#include <string.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <pthread.h>
#include "block_queue.h"

#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::get_instance()->write_log(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__)

class Log{
public:
    // 单例模式
    static Log* get_instance(){
        static Log instance;
        return &instance;
    }
    static void *flush_log_thread(void *args){
        Log::get_instance()->asyn_write_log();
    }
    bool init(const char *file_name, int mlog_buf_size = 8192, int m_max_lines = 5000000, int max_queue_size = 0);
    void write_log(int level, const char *format, ...);
    void flush(void);
private:
    Log(){
        m_count = 0;
        m_is_async = false;
    }
    ~Log(){
        if(m_fp != NULL){
            fclose(m_fp);
        }
    }

    void *asyn_write_log(){
        std::string single_log;
        while(m_log_queue->pop(single_log)){
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }
private:
    locker m_mutex;
    block_queue<string> *m_log_queue;   //阻塞队列

    char dir_name[128];
    char log_name[128];

    char *m_buf;    // 日志缓冲区
    int m_log_buf_size; // 缓冲区大小
    int m_max_lines;  // 日志最大行数
    long long m_count;  // 记录日志行数
    int m_today;    // 记录是哪一天的日志
    FILE *m_fp;  // 打开log的文件指针
    bool m_is_async; //是否异步日志


};
#endif