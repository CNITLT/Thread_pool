#include "Thread.h"
#include <stdio.h>
#include <stdlib.h>
#include "Thread_pool.h"
void* Thread::__routine(void* arg){
    pthread_detach(pthread_self());
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    
    Thread* mythis = (Thread*)arg;
    Thread_pool& ref_master_thread_pool= *mythis->__m_p_master_thread_pool;
    Task& ref_task = mythis->__m_task;
    
    for(;;){
        ref_master_thread_pool.get_task(&ref_task, mythis->get_thread_id());
        ref_task.run();
        ref_master_thread_pool.add_task_log(&ref_task,mythis->get_thread_id());
    }
}

Thread::Thread(Thread_pool* p_master_thread_pool){
    this->__m_p_master_thread_pool = p_master_thread_pool;
    int err = pthread_create(&this->__m_thread_id,NULL,
                (Thread_func)Thread::__routine,this);
    if(err != 0){
        fprintf(stderr, "pthread_create fail, errno:%d\n", err);
        exit(-1);
    }
}

Thread* Thread::create_thread(Thread_pool* p_master_thread_pool){
    return new Thread(p_master_thread_pool);
}

pthread_t Thread::get_thread_id(){
    return this->__m_thread_id;
}


