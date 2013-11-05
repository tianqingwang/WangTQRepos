#ifndef __WORKQUEUE_H__
#define __WORKQUEUE_H__

#include <pthread.h>
#include <sys/time.h>


typedef struct job{
    void *(*job_function)(void *arg); /*callback function definition*/
    void *arg;
    struct job *next;
}job_t;

typedef struct workqueue{
    job_t              *jobs_head;
    job_t              *jobs_tail;
    
    int                shutdown;
    pthread_t          *threadid;
    int                max_thread_num;
    int                cur_queue_size;
    
    pthread_mutex_t    jobs_mutex;
    pthread_cond_t     jobs_notempty;
    pthread_cond_t     jobs_empty;
}workqueue_t;

int  workqueue_init(int nworks);
void workqueue_shutdown();
void workqueue_add_job(void *(*job_function)(void *arg),void *arg);
#endif
