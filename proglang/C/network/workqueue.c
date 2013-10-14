/****************************************************
* Multi-threads work queue.
* Author: Wang Tianqing
* Date  : 2013-09-18
*****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "workqueue.h"

/*********************************************************
*           WORKQUEUE MODEL PICTURE
*   --------------------------  workqueue
*   |      |     |    |     |
*   | job0 |job1 |....|jobN |---------
*   |      |     |    |     |        |
*   --------------------------       |
*                                   \|/
*   --------------------------------------------- thread pool
*   |      ---------  ---------     ---------   |
*   |      |thread1|  |thread2| ... |threadM|   | 
*   |      |       |  |       |     |       |   |
*   |      ---------  ---------     ---------   |
*   ---------------------------------------------
**********************************************************/
#if 0
#define LIST_ADD(item,list){  \
    item->prev = NULL;               \
    item->next = list;               \
    list = item;                     \
}


#define LIST_REMOVE(item,list){  \
    if (item->prev != NULL) item->prev->next = item->next; \
    if (item->next != NULL) item->next->prev = item->prev; \
    if (item == list) list = item->next;                   \
    item->prev = item->next = NULL;                        \
}
#endif

#if 1
#define LIST_INIT_HEAD(head){                 \
    head->next = head;                        \
    head->prev = head;                        \
}

#define LIST_ADD(item,head){                     \
    head->next->prev = item;                     \
    item->next       = head->next;               \
    item->prev       = head;                     \
    head->next       = item;                     \
}

#define LIST_REMOVE(item,head){                  \
    printf("head=%p\n",head);                    \
    if (head->next == head){                     \
        item = NULL;                             \
    }                                            \
    else{                                        \
        item = head->prev;                       \
        head->prev  = item->prev;                \
        item->prev->next = head;                 \
        item->next = item->prev = NULL;          \
    }                                            \
}
#endif

#if 0



#endif

static worker_t worker_head;
static job_t    job_head;

pthread_mutex_t jobs_mutex;
pthread_cond_t  jobs_cond;

job_t *workqueue_fetch_job(workqueue_t *workqueue);

static void *worker_function(void *args)
{
    worker_t *worker = (worker_t*)args;
    job_t   *job;
    
    while(1){    
        job = workqueue_fetch_job(worker->workqueue);
        
        if (worker->shutdown) break;
        
        if (job == NULL) continue;
        
        /*execute job*/
        job->job_function(job);
    }
    
    free(worker);
    pthread_exit(NULL);
}

/*purpose: create thread pool to run the jobs in workqueue*/
int  workqueue_init(workqueue_t *workqueue,int nworks)
{
    int i;
    worker_t *worker;
    
    pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  init_cond  = PTHREAD_COND_INITIALIZER;
    
    if (workqueue == NULL){
        return -1;
    }
    
    if (nworks < 0) nworks = 1;
    
    memset(workqueue,0,sizeof(workqueue_t));
//    memcpy(&workqueue->jobs_mutex,&init_mutex,sizeof(pthread_mutex_t));
//    memcpy(&workqueue->jobs_cond,&init_cond,sizeof(pthread_cond_t));
    
    memcpy(&jobs_mutex,&init_mutex,sizeof(pthread_mutex_t));
    memcpy(&jobs_cond,&init_cond,sizeof(pthread_cond_t));
    
    /*workqueue heads*/
    workqueue->workers = &worker_head;
    workqueue->waiting_jobs = &job_head;
    
    LIST_INIT_HEAD(workqueue->workers);
    LIST_INIT_HEAD(workqueue->waiting_jobs);
    
    for (i=0; i<nworks; i++){
        worker = (worker_t*)malloc(sizeof(worker_t));
        memset(worker,0,sizeof(worker_t));
        
        worker->workqueue = workqueue;
        if (pthread_create(&worker->tid,NULL,worker_function,(void*)worker) != 0){
            /*failed*/
            perror("failed to allocate workers");
            free(worker);
            return -1;
        }
        
        /*add worker to queue*/
        LIST_ADD(worker,worker->workqueue->workers);
    }
}


/*purpose: shutdown thread pool, free workqueue*/
void  workqueue_shutdown(workqueue_t *workqueue)
{
    worker_t *worker;
    
    for (worker = workqueue->workers; worker != NULL; worker = worker->next){
        worker->shutdown = 1;
    }
    
    pthread_mutex_lock(&workqueue->jobs_mutex);
    workqueue->workers = NULL;
    workqueue->waiting_jobs    = NULL;
    pthread_cond_broadcast(&workqueue->jobs_cond);
    pthread_mutex_unlock(&workqueue->jobs_mutex);
}

#if 0
/*purpose: add a new job to workqueue*/
void  workqueue_add_job(workqueue_t *workqueue,job_t *job)
{
    pthread_mutex_lock(&workqueue->jobs_mutex);
    /*add job to waiting_jobs*/
    LIST_ADD(job,workqueue->waiting_jobs);
    pthread_cond_signal(&workqueue->jobs_cond);
    pthread_mutex_unlock(&workqueue->jobs_mutex);
}
#else
void workqueue_add_job(workqueue_t *workqueue, job_t *job)
{
    pthread_mutex_lock(&jobs_mutex);
    LIST_ADD(job,workqueue->waiting_jobs);
    pthread_cond_signal(&jobs_cond);
    pthread_mutex_unlock(&jobs_mutex);
}

#endif

job_t *workqueue_fetch_job(workqueue_t *workqueue)
{
    job_t *job;
    
    pthread_mutex_lock(&jobs_mutex);
    
    while (workqueue->waiting_jobs->next == workqueue->waiting_jobs){
        //pthread_cond_wait(&worker->workqueue->jobs_cond,&worker->workqueue->jobs_mutex);
        pthread_cond_wait(&jobs_cond,&jobs_mutex);
//        pthread_mutex_unlock(&jobs_mutex);
//        return NULL;
    }
    
    LIST_REMOVE(job,workqueue->waiting_jobs);
    
    pthread_mutex_unlock(&jobs_mutex);
    
    return job;
}