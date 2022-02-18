#include "Thread_pool.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "debug.h"
void* forn(void* num){
	unsigned int i = 0;
    unsigned int n = *((unsigned int*)&num);
    printf("pthread:%u run num:%u\n", pthread_self(), num);
    for(;i<n;i++){
        printf("pthread:%u i:%d\n", pthread_self(), i);
    }
    printf("pthread:%u end num:%u\n", pthread_self(), num);
    
    return NULL;
}
int main(int argc, char* argv[]){
    if(argc < 3){
        printf("usage:test threadNum forNumber\n");
        exit(-1);
    }
    srand(time(NULL));
    int threadNum = atoi(argv[1]);
    int i = 0;
    Thread_pool* p_pool = Thread_pool::create_thread_pool(1, threadNum);
    
    Task_id task_ids[threadNum];
    for(i = 0;i<threadNum;i++){
       p_pool->add_task(forn, (void*)(atoi(argv[2])),task_ids + i);
    }
    for(i = 0;i<threadNum;i++){
       p_pool->wait(task_ids[i]);
    }
    printf("run thread:%d\n", p_pool->get_run_thread_num());
    printf("wait thread:%d\n", p_pool->get_wait_thread_num());
    printf("while destory\n",i);
    Thread_pool::destory_thread_pool(p_pool);
	return 0;
}
