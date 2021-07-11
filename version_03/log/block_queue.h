#ifndef _BLOCK_QUEUE_H_
#define _BLOCK_QUEUE_H_

#include <iostream>
#include <pthread.h>
#include <string>
#include "../lock/lock.h"
using namespace std;

// TODO: 尝试给阻塞队列添加扩容机制

template<class T>
class block_queue{

public:
    block_queue(int max_size = 1000){
        if(max_size <= 0){
            exit(-1);
        }

        m_max_size = max_size;
        m_array = new T[m_max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    ~block_queue(){
        m_mutex.lock();
        if(m_array != NULL){
            delete []m_array;
        }
        m_mutex.unlock();
    }
public:
    void clear(){
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    int max_size(){
        int tmp;
        m_mutex.lock();
        tmp = m_max_size;
        m_mutex.unlock();
        return tmp;
    }

    int size(){
        int tmp;
        m_mutex.lock();
        tmp = m_size;
        m_mutex.unlock();
        return tmp;
    }

    bool full(){
        m_mutex.lock();
        if(m_size >= m_max_size){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool empty(){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool front(T &value){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    bool back(T &value){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }    

    bool push(const T& value){
        m_mutex.lock();
        // 队列满的时候, 通知消费者赶紧消费
        if(m_size >= m_max_size){
            m_mutex.unlock();
            m_cond.broadcast();
            return false;
        }

        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = value;
        m_size++;

        m_mutex.unlock();
        m_cond.broadcast();
        return true;
    }

    bool pop(T& value){
        m_mutex.lock();
        while(m_size <= 0){
            if(!m_cond.wait(m_mutex.get())){
                m_mutex.unlock();
                return false;
            }
        }

        // 循环队列头部指针指的是front的前一个
        // 尾部指针指的是back
        m_front = (m_front + 1) % m_max_size;
        value = m_array[m_front];
        m_size--;

        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex;     // 定义互斥锁, 锁住共享队列
    cond m_cond;        // 定义条件变量, 配合mutex

    T *m_array;         // 定义模拟循环队列的数组
    int m_size;         // 队列中的元素个数
    int m_max_size;     // 队列中最大元素个数

    // 循环队列头部指针指的是front的前一个
    // 尾部指针指的是back
    int m_front;          // 队列头指针
    int m_back;           // 队列尾指针
};

#endif