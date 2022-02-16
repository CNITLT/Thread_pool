#ifndef THREAD_H
#define THREAD_H
#include "Task.h"
#include <pthread.h>
class Thread_pool;

class Thread{
private:
    Task __m_task;
    Thread_pool* __m_p_master_thread_pool;
    pthread_t __m_thread_id;
private:
    static void* __routine(void* arg);
    Thread(Thread_pool* p_master_thread_pool);
public:
    static Thread* create_thread(Thread_pool* p_master_thread_pool);
    pthread_t get_thread_id();
};
#endif