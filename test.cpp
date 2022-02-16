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
        //printf("pthread:%u i:%d\n", pthread_self(), i);
    }
     printf("pthread:%u end num:%u\n", pthread_self(), num);
    
    return NULL;
}
int main(int argc, char* argv[]){
    srand(time(NULL));
    int threadNum = atoi(argv[1]);
    int i = 0;
    Thread_pool* p_pool = Thread_pool::create_thread_pool(2, 50);
    
    for(;i<threadNum;i++){
       p_pool->add_task(forn, (void*)(atoi(argv[2])*rand()),NULL);
    }
    
    char buff[1024];
    i = 1;
    int task = 0;
	while(i++){ 
        p_pool->add_task(forn, (void*)(atoi(argv[2])*rand()),NULL);
        DEBUG_INFO("add task task num:%d\n",p_pool->get_task_num());
        sleep(1); 
    }
    printf("run thread:%d\n", p_pool->get_run_thread_num());
    printf("wait thread:%d\n", p_pool->get_wait_thread_num());
    printf("while destory\n",i);
    Thread_pool::destory_thread_pool(p_pool);
	return 0;
}
