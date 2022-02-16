#ifndef TASK_H
#define TASK_H
#include <pthread.h>
#include <time.h>
#include <string.h>


typedef void *(*Thread_func)(void *);
class Thread;
class Thread_pool;

typedef unsigned int Task_id;
class Task{
private:
    Task_id __m_task_id;
    Thread_func __m_func;
    void *__m_arg;
    void *__m_result;
    timeval __m_create_time;
    timeval __m_start_time;
    timeval __m_end_time;
public:
    Task(Thread_func func, void *arg, Task_id task_id);
    Task() = default;
    void record_start_time();
    void record_end_time();
    void set_result(void* result);
    void* get_result();
    void get_create_time(timeval* p_time);
    void get_run_time(timeval* p_time);
    void* run();
    friend Thread;
    friend Thread_pool;
};

typedef Task Task_log;
#endif