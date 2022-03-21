#include "Thread_pool.h"
#include <stdio.h>
#include <sys/resource.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include "debug.h"
const size_t Thread_pool::__get_cpu_thread_num(){
    FILE* fp = popen("grep 'processor' /proc/cpuinfo | sort -u | wc -l","r");
    size_t cpu_thread_num = 0;
    fscanf(fp,"%d",&cpu_thread_num);
    pclose(fp);
    return cpu_thread_num;
}
const size_t Thread_pool::__get_max_thread_num(){
    // FILE* fp = popen("ulimit -u","r");
    // size_t max_thread_num = 0;
    // fscanf(fp,"%d",&max_thread_num);
    // pclose(fp);
    // return max_thread_num;
    return __get_cpu_thread_num() * 2;
}

const size_t Thread_pool::CPU_THREAD_NUM = __get_cpu_thread_num();
const size_t Thread_pool::DEFAULT_MIN_THREAD_NUM = Thread_pool::CPU_THREAD_NUM + 1;
const size_t Thread_pool::DEFAULT_MAX_THREAD_NUM = __get_max_thread_num();

Thread_pool::Thread_pool(size_t min_thread_num, size_t max_thread_num){
    if(min_thread_num < DEFAULT_MIN_THREAD_NUM){
        min_thread_num = DEFAULT_MIN_THREAD_NUM;
    }
    if(min_thread_num > DEFAULT_MAX_THREAD_NUM){
        min_thread_num = DEFAULT_MAX_THREAD_NUM;
    }
    if(max_thread_num < min_thread_num){
        max_thread_num = min_thread_num;
    }
    if(max_thread_num > DEFAULT_MAX_THREAD_NUM){
        max_thread_num = DEFAULT_MAX_THREAD_NUM;
    }
    
    this->__m_min_thread_num = min_thread_num;
    this->__m_max_thread_num = max_thread_num;
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&this->__m_mutex_thread_num, &mutexattr);
    pthread_mutex_init(&this->__m_mutex_thread_map, &mutexattr);
    pthread_mutex_init(&this->__m_mutex_task_queue, &mutexattr);
    pthread_mutex_init(&this->__m_mutex_task_log_map, &mutexattr);
    this->__m_can_add_task = true;
    this->__m_task_id_counter = 1;
    this->__m_cond_task_queue = PTHREAD_COND_INITIALIZER;
    this->__m_cond_wait_task = PTHREAD_COND_INITIALIZER;
    int i = 0;
    pthread_mutex_lock(&this->__m_mutex_thread_map);
    for(;i<this->__m_min_thread_num;i++){
        Thread* p_thread = Thread::create_thread(this);
        this->__m_wait_thread_map.insert(
            std::pair<pthread_t, Thread*>(
            p_thread->get_thread_id(), p_thread)
            );
    }
    pthread_mutex_unlock(&this->__m_mutex_thread_map);
}

Thread_pool::~Thread_pool(){
    pthread_mutex_lock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u lock __m_mutex_task_queue in ~Thread_pool\n", pthread_self());
    this->__m_can_add_task = false;
    pthread_mutex_unlock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u unlock __m_mutex_task_queue in ~Thread_pool\n", pthread_self());

   
    pthread_mutex_lock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u lock __m_mutex_thread_map in ~Thread_pool\n", pthread_self());
    //让线程取消
    for(auto iter:this->__m_wait_thread_map){
        pthread_cancel(iter.second->get_thread_id());
        DEBUG_INFO("pthread:%u call pthread_cancel(%u) in ~Thread_pool\n", pthread_self(),iter.second->get_thread_id());
    }
    for(auto iter:this->__m_run_thread_map){
        pthread_cancel(iter.second->get_thread_id());
        DEBUG_INFO("pthread:%u call pthread_cancel(%u) in ~Thread_pool\n", pthread_self(),iter.second->get_thread_id());
    }
    
    pthread_mutex_unlock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u unlock __m_mutex_thread_map in ~Thread_pool\n", pthread_self());
    pthread_mutex_unlock(&this->__m_mutex_thread_num);
    pthread_mutex_unlock(&this->__m_mutex_task_queue);
    pthread_mutex_unlock(&this->__m_mutex_task_log_map);
    DEBUG_INFO("pthread:%u unlock all mutex in ~Thread_pool\n", pthread_self());
    
    pthread_mutex_destroy(&this->__m_mutex_thread_map);
    pthread_mutex_destroy(&this->__m_mutex_thread_num);
    pthread_mutex_destroy(&this->__m_mutex_task_queue);
    pthread_mutex_destroy(&this->__m_mutex_task_log_map);
    DEBUG_INFO("pthread:%u destory all mutex in ~Thread_pool\n", pthread_self());
    pthread_cond_destroy(&this->__m_cond_task_queue);
    pthread_cond_destroy(&this->__m_cond_wait_task);
    DEBUG_INFO("pthread:%u destory all cond in ~Thread_pool\n", pthread_self());
}


 Thread_pool* Thread_pool::
 create_thread_pool(size_t min_thread_num ,size_t max_thread_num){
     return new Thread_pool(min_thread_num,max_thread_num);
 }
 
 void Thread_pool::destory_thread_pool(Thread_pool* p_thread_pool){
     delete p_thread_pool;
 }

//get在两次读取之间可能会产生不可重复读， 问题不大，但可以避免被阻塞
size_t Thread_pool::get_min_thread_num(){
    return this->__m_min_thread_num;
}
size_t Thread_pool::set_min_thread_num(size_t min_thread_num){
    if(min_thread_num < DEFAULT_MIN_THREAD_NUM){
        min_thread_num = DEFAULT_MIN_THREAD_NUM;
    }
    if(min_thread_num > this->__m_max_thread_num){
        min_thread_num = this->__m_max_thread_num;
    }
    pthread_mutex_lock(&this->__m_mutex_thread_num);
    DEBUG_INFO("pthread:%u lock __m_mutex_thread_num in set_min_thread_num\n",pthread_self());
    this->__m_min_thread_num = min_thread_num;
    pthread_mutex_unlock(&this->__m_mutex_thread_num);
    DEBUG_INFO("pthread:%u unlock __m_mutex_thread_num in set_min_thread_num\n",pthread_self());
    this->__adjust_thread_num();
}
size_t Thread_pool::get_max_thread_num(){
    return this->__m_max_thread_num;
}
size_t Thread_pool::set_max_thread_num(size_t max_thread_num){
    if(max_thread_num > DEFAULT_MAX_THREAD_NUM){
        max_thread_num = DEFAULT_MAX_THREAD_NUM;
    }
    if(max_thread_num < this->__m_min_thread_num){
        max_thread_num = this->__m_min_thread_num;
    }
    pthread_mutex_lock(&this->__m_mutex_thread_num);
    DEBUG_INFO("pthread:%u lock __m_mutex_thread_num in set_max_thread_num\n",pthread_self());
    this->__m_max_thread_num = max_thread_num;
    pthread_mutex_unlock(&this->__m_mutex_thread_num);
    DEBUG_INFO("pthread:%u unlock __m_mutex_thread_num in set_max_thread_num\n",pthread_self());
    this->__adjust_thread_num();
}
size_t Thread_pool::get_current_thread_num(){
    return this->__m_run_thread_map.size() + this->__m_wait_thread_map.size();
}
size_t Thread_pool::get_run_thread_num(){
    return this->__m_run_thread_map.size();
}
size_t Thread_pool::get_wait_thread_num(){
    return this->__m_wait_thread_map.size();
}
size_t Thread_pool::get_task_num(){
    return this->__m_user_task_queue.size();
}



void Thread_pool::__adjust_thread_num(){
    pthread_mutex_lock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u lock __m_mutex_task_queue in __adjust_thread_num\n", pthread_self());
    pthread_mutex_lock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u lock __m_mutex_thread_map in __adjust_thread_num\n", pthread_self());
    struct rusage rus;
    getrusage(RUSAGE_SELF, &rus);
    bool prefer_io_flag = rus.ru_nvcsw > rus.ru_nivcsw;
    bool prefer_cpu_flag = rus.ru_nvcsw < rus.ru_nivcsw;
    if(this->get_task_num() > this->get_wait_thread_num()
       &&this->get_current_thread_num() < this->__m_max_thread_num
       &&prefer_io_flag){
        //增加一个线程
        DEBUG_INFO("add thread\n");
        Thread* p_thread = Thread::create_thread(this);
        this->__m_wait_thread_map.insert(
            std::pair<pthread_t, Thread*>(
            p_thread->get_thread_id(), p_thread)
            );
    }
    else if(this->get_task_num() < this->get_wait_thread_num()
            &&this->get_current_thread_num() > this->__m_min_thread_num
            &&prefer_cpu_flag){
        //减少一个线程
        DEBUG_INFO("delete thread\n");
        auto iter = this->__m_wait_thread_map.begin();
        if(iter->first == pthread_self()){
            this->__m_wait_thread_map.erase(iter);
            pthread_mutex_unlock(&this->__m_mutex_thread_map); 
            DEBUG_INFO("pthread:%u unlock __m_mutex_thread_map in __adjust_thread_num\n", pthread_self());
            pthread_mutex_unlock(&this->__m_mutex_task_queue);
            DEBUG_INFO("pthread:%u unlock __m_mutex_task_queue in __adjust_thread_num\n", pthread_self());

            DEBUG_INFO("pthread:%u will exit in __adjust_thread_num\n", pthread_self());
            pthread_exit(NULL);
        }
        else{
            this->__m_wait_thread_map.erase(iter);
            pthread_cancel(iter->first);
            DEBUG_INFO("pthread:%u call pthread_cancel(%u) in __adjust_thread_num\n", pthread_self(),iter->first);
        }
    }
    pthread_mutex_unlock(&this->__m_mutex_thread_map); 
    DEBUG_INFO("pthread:%u unlock __m_mutex_thread_map in __adjust_thread_num\n", pthread_self());
    pthread_mutex_unlock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u unlock __m_mutex_task_queue in __adjust_thread_num\n", pthread_self());
}

void Thread_pool::get_task(Task* p_task, pthread_t thread_id){
    pthread_mutex_lock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u lock __m_mutex_task_queue in get_task\n", pthread_self());
    while(this->__m_user_task_queue.size() <= 0){
        DEBUG_INFO("pthread:%u unlock __m_mutex_task_queue before wait in get_task\n", pthread_self());
        pthread_cond_wait(&this->__m_cond_task_queue, 
                        &this->__m_mutex_task_queue);
        DEBUG_INFO("pthread:%u lock __m_mutex_task_queue after wait in get_task\n", pthread_self());
    }
    *p_task = this->__m_user_task_queue.front();
    this->__m_user_task_queue.pop();
    pthread_mutex_unlock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u unlock __m_mutex_task_queue in get_task\n", pthread_self());
    pthread_mutex_lock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u lock __m_mutex_thread_map in get_task\n", pthread_self());
    auto iter = this->__m_wait_thread_map.find(thread_id);
    if(iter == this->__m_wait_thread_map.end()){
        fprintf(stderr,"get_task fail\n");
        exit(-1);
    }
    this->__m_run_thread_map.insert(*iter);
    this->__m_wait_thread_map.erase(iter);
    pthread_mutex_unlock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u unlock __m_mutex_thread_map in get_task\n", pthread_self());
}

void Thread_pool::add_task_log(Task_log* p_task, pthread_t thread_id){
    pthread_mutex_lock(&this->__m_mutex_task_log_map);
    DEBUG_INFO("pthread:%u lock __m_mutex_task_log_map in add_task_log\n", pthread_self());
    this->__m_task_log_map.insert(
        std::pair<Task_id,Task_log>(p_task->__m_task_id,*p_task));
    pthread_mutex_unlock(&this->__m_mutex_task_log_map);
    DEBUG_INFO("pthread:%u unlock __m_mutex_task_log_map in add_task_log\n", pthread_self());
    pthread_cond_signal(&this->__m_cond_wait_task);

    pthread_mutex_lock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u lock __m_mutex_thread_map in add_task_log\n", pthread_self());
    auto iter = this->__m_run_thread_map.find(thread_id);
    if(iter == this->__m_run_thread_map.end()){
        fprintf(stderr,"add_task_log fail\n");
        exit(-1);
    }
    this->__m_wait_thread_map.insert(*iter);
    this->__m_run_thread_map.erase(iter);
    pthread_mutex_unlock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u unlock __m_mutex_thread_map in add_task_log\n", pthread_self());
    
    this->__adjust_thread_num();
}
int Thread_pool::add_task(Thread_func func, void* arg, Task_id* p_task_id){
    if(!this->__m_can_add_task){
        return -1;
    }
    pthread_mutex_lock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u lock __m_mutex_task_queue in add_task\n", pthread_self());
    if(!this->__m_can_add_task){
        pthread_mutex_unlock(&this->__m_mutex_task_queue);
        DEBUG_INFO("pthread:%u unlock __m_mutex_task_queue in add_task\n", pthread_self());
        return -1;
    }
    Task_id task_id = this->__m_task_id_counter++;
    this->__m_user_task_queue.push(Task(func,arg,task_id));
    pthread_mutex_unlock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u unlock __m_mutex_task_queue in add_task\n", pthread_self());
    pthread_cond_signal(&this->__m_cond_task_queue);
    if(p_task_id){
        *p_task_id = task_id;
    }
    this->__adjust_thread_num();
    return 0;
}
void* Thread_pool::get_result(Task_id task_id){
    void* result=NULL;
    pthread_mutex_lock(&this->__m_mutex_task_log_map);
    DEBUG_INFO("pthread:%u lock __m_mutex_task_log_map in get_result\n", pthread_self());
    auto iter = this->__m_task_log_map.find(task_id);
    if(iter != this->__m_task_log_map.end()){
        result = iter->second.get_result();
    }
    pthread_mutex_unlock(&this->__m_mutex_task_log_map);
    DEBUG_INFO("pthread:%u unlock __m_mutex_task_log_map in get_result\n", pthread_self());
    return result;
}

int Thread_pool::wait(Task_id task_id){
    bool in_log_map = false;
    pthread_mutex_lock(&this->__m_mutex_task_log_map);
    DEBUG_INFO("pthread:%u lock __m_mutex_task_log_map in wait\n", pthread_self());
    auto iter = this->__m_task_log_map.find(task_id);
    if(iter != __m_task_log_map.end()){
        in_log_map = true;
    }
    pthread_mutex_unlock(&this->__m_mutex_task_log_map);
    DEBUG_INFO("pthread:%u unlock __m_mutex_task_log_map in wait\n", pthread_self());
    //已经完成的直接返回0
    if(in_log_map){
        return 0;
    }
    bool is_wait = false;
    //查看下是不是在运行或者是在任务队列里等待运行，是的话进行等待，不是的话返回-1
    pthread_mutex_lock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u lock __m_mutex_thread_map in wait\n", pthread_self());
    for(auto iter:this->__m_run_thread_map){
        if(task_id == iter.second->__m_task.__m_task_id){
            is_wait = true;
            break;
        }
    }
    pthread_mutex_unlock(&this->__m_mutex_thread_map);
    DEBUG_INFO("pthread:%u unlock __m_mutex_thread_map in wait\n", pthread_self());
    
    pthread_mutex_lock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u lock __m_mutex_task_queue in wait\n", pthread_self());
    int queue_size = this->__m_user_task_queue.size();
    //这个效率有点低，以后有时间再改改
    for(int i = 0; i < queue_size; i++) {   
        if(this->__m_user_task_queue.front().__m_task_id == task_id){
            is_wait = true;
        }
        this->__m_user_task_queue.push(this->__m_user_task_queue.front());
        this->__m_user_task_queue.pop();
    } 
    pthread_mutex_unlock(&this->__m_mutex_task_queue);
    DEBUG_INFO("pthread:%u unlock __m_mutex_task_queue in wait\n", pthread_self());
   
    if(is_wait){
        pthread_mutex_lock(&this->__m_mutex_task_log_map);
        DEBUG_INFO("pthread:%u lock __m_mutex_task_log_map in wait\n", pthread_self());
        while(this->__m_task_log_map.find(task_id) == 
             this->__m_task_log_map.end()){
            pthread_cond_wait(&this->__m_cond_wait_task,
                            &this->__m_mutex_task_log_map);
        }
        pthread_mutex_unlock(&this->__m_mutex_task_log_map);
        DEBUG_INFO("pthread:%u unlock __m_mutex_task_log_map in wait\n", pthread_self());
        return 0;
    }
    
    return -1;
}