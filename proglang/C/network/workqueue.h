#ifndef __WORKQUEUE_H__
#define __WORKQUEUE_H__

#include <pthread.h>

typedef struct user_data_s{
    int fd;
    int datalen;
    char *pdata;
    struct timeval accept_time;
}user_data_t;

typedef struct job{
    void *(*job_function)(void *arg); /*callback function definition*/
    void *arg;
    struct job *next;
}job_t;

typedef struct workqueue{
    struct worker   *workers;
    
    job_t *jobs_head;
    job_t *jobs_tail;
    
    int shutdown;
    pthread_t *threadid;
    int max_thread_num;
    int cur_queue_size;
    
    pthread_mutex_t  jobs_mutex;
    pthread_cond_t   jobs_cond;
}workqueue_t;

int  workqueue_init(int nworks);
void workqueue_shutdown();
void workqueue_add_job(void *(*job_function)(void *arg),void *arg);

void         init_datalist(int nMaxConnection);
user_data_t *get_from_datalist();
void         add_to_datalist(user_data_t *user_data);
void         free_datalist();

#endif
