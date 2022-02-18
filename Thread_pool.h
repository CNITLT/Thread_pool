#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include "Thread.h"
#include <map>
#include <queue>
using std::queue;
using std::map;

class Thread_pool final{
protected:
    static const size_t __get_cpu_thread_num();
    static const size_t __get_max_thread_num();
    //CPU线程数，用于动态调整线程数
    static const size_t CPU_THREAD_NUM;
    //最小线程数, CPU线程数+1
    static const size_t DEFAULT_MIN_THREAD_NUM;
    //最大线程数
    static const size_t DEFAULT_MAX_THREAD_NUM;
private:
    size_t __m_min_thread_num;
    size_t __m_max_thread_num; 
    pthread_mutex_t __m_mutex_thread_num;

    queue<Task> __m_user_task_queue;
    bool __m_can_add_task;
    Task_id __m_task_id_counter;
    pthread_mutex_t __m_mutex_task_queue;
    pthread_cond_t __m_cond_task_queue;

    map<Task_id, Task_log> __m_task_log_map;
    pthread_mutex_t __m_mutex_task_log_map;
    pthread_cond_t __m_cond_wait_task;
    
    map<pthread_t, Thread*> __m_run_thread_map;
    map<pthread_t, Thread*> __m_wait_thread_map;
    pthread_mutex_t __m_mutex_thread_map;
    
private:
    void __adjust_thread_num();
    Thread_pool(size_t min_thread_num = DEFAULT_MIN_THREAD_NUM, 
                size_t max_thread_num = DEFAULT_MAX_THREAD_NUM);
    ~Thread_pool();
public:
    static Thread_pool* create_thread_pool(size_t min_thread_num = DEFAULT_MIN_THREAD_NUM, 
                                        size_t max_thread_num = DEFAULT_MAX_THREAD_NUM);
    static void destory_thread_pool(Thread_pool* p_thread_pool);
    size_t get_min_thread_num();
    size_t set_min_thread_num(size_t min_thread_num);
    size_t get_max_thread_num();
    size_t set_max_thread_num(size_t max_thread_num);
    size_t get_current_thread_num();
    size_t get_run_thread_num();
    size_t get_wait_thread_num();
    size_t get_task_num();
    void get_task(Task* p_task, pthread_t thread_id);
    void add_task_log(Task_log* p_task, pthread_t thread_id);
    /*失败返回-1*/
    int add_task(Thread_func func, void* arg, Task_id* p_task_id);
    void* get_result(Task_id task_id);

    /*失败返回-1，说明没有该任务*/
    int wait(Task_id task_id);
};
#endif