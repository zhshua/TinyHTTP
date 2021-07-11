#ifndef _LOCK_H_
#define _LOCK_H_

#include <pthread.h>
#include <semaphore.h>
#include <exception>

class sem{
public:
    sem(int num){
        // sem_init(), 第二个参数, 传0代表用于线程, 传1代表用于进程
        if(sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    sem(){
        // sem_init(), 第二个参数, 传0代表用于线程, 传1代表用于进程
        if(sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    // 析构函数不抛异常, c++默认是析构函数无异常的
    ~sem(){
        sem_destroy(&m_sem);
    }
    // 对信号量做--
    bool wait(){
        return sem_wait(&m_sem) == 0;
    }
    // 对信号量做++
    bool post(){
        return sem_post(&m_sem) == 0;
    }
private:
    sem_t m_sem;
};

class locker{
public:
    locker(){
        if(pthread_mutex_init(&mutex, NULL) != 0){
            throw std::exception();
        }
    }
    ~locker(){
        pthread_mutex_destroy(&mutex);
    }
    bool lock(){
        return pthread_mutex_lock(&mutex) == 0;
    }
    bool unlock(){
        return pthread_mutex_unlock(&mutex) == 0;
    }
    pthread_mutex_t *get(){
        return &mutex;
    }
private:
    pthread_mutex_t mutex;
};

class cond{
public:
    cond(){
        if(pthread_cond_init(&m_cond, NULL) != 0){
            throw std::exception();
        }
    }
    ~cond(){
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t *mutex){
        return pthread_cond_wait(&m_cond, mutex) == 0;
    }
    bool signal(){
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond) == 0;
    }
private:
    pthread_cond_t m_cond;
};

#endif