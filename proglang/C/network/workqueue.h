#ifndef __WORKQUEUE_H__
#define __WORKQUEUE_H__

#include <pthread.h>

typedef struct job{
    void (*job_function)(struct job *jobs); /*callback function definition*/
    void *user_data;
    struct job *prev;
    struct job *next;
}job_t;

typedef struct worker{
    pthread_t            tid;
    int                  shutdown;    /*control the running threads to exit*/
    struct workqueue    *workqueue;
    struct worker       *prev;
    struct worker       *next;
}worker_t;

typedef struct workqueue{
    struct worker   *workers;
    struct job      *waiting_jobs;
    pthread_mutex_t  jobs_mutex;
    pthread_cond_t   jobs_cond;
}workqueue_t;

int  workqueue_init(workqueue_t *workqueue,int nworks);
void workqueue_shutdown(workqueue_t *workqueue);
void workqueue_add_job(workqueue_t *workqueue,job_t *job);

#endif
