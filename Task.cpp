#include "Task.h"
#include <sys/time.h>
Task::Task(Thread_func func, void *arg, Task_id task_id){
    this->__m_func = func;
    this->__m_arg = arg;
    this->__m_task_id = task_id;
    this->__m_result = NULL;
    gettimeofday(&this->__m_create_time,NULL);
    bzero(&this->__m_start_time, sizeof(this->__m_start_time));
    bzero(&this->__m_end_time, sizeof(this->__m_end_time)); 
}

void Task::record_start_time(){
     gettimeofday(&this->__m_start_time,NULL);
}
void Task::record_end_time(){
     gettimeofday(&this->__m_end_time,NULL);
}
void Task::set_result(void* result){
    this->__m_result = result;
}
void* Task::get_result(){
    return this->__m_result;
}
void Task::get_create_time(timeval* p_time){
    memcpy(p_time,&this->__m_create_time,sizeof(*p_time));
}
void Task::get_run_time(timeval* p_time){
    p_time->tv_sec = this->__m_end_time.tv_sec - this->__m_start_time.tv_sec;
    p_time->tv_usec = this->__m_end_time.tv_usec - this->__m_start_time.tv_usec;
    if(p_time->tv_usec < 0){
        p_time->tv_usec += 1000*1000;
        p_time->tv_sec--;
    }
}

void* Task::run(){
    this->record_start_time();
    this->__m_result= this->__m_func(this->__m_arg);
    this->record_end_time();
}