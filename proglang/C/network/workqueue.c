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

static void *worker_function(void *args)
{
    worker_t *worker = (worker_t*)args;
    job_t   *job;
    
    while(1){
        pthread_mutex_lock(&worker->workqueue->jobs_mutex);
        while(worker->workqueue->waiting_jobs == NULL){
            /*blocked to wait cond signal*/
            pthread_cond_wait(&worker->workqueue->jobs_cond,&worker->workqueue->jobs_mutex);
        }
        
        job = worker->workqueue->waiting_jobs;
        if (job != NULL){
            LIST_REMOVE(job,worker->workqueue->waiting_jobs);
        }
        pthread_mutex_unlock(&worker->workqueue->jobs_mutex);
        
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
    memcpy(&workqueue->jobs_mutex,&init_mutex,sizeof(pthread_mutex_t));
    memcpy(&workqueue->jobs_cond,&init_cond,sizeof(pthread_cond_t));
    
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

/*purpose: add a new job to workqueue*/
void  workqueue_add_job(workqueue_t *workqueue,job_t *job)
{
    pthread_mutex_lock(&workqueue->jobs_mutex);
    /*add job to waiting_jobs*/
    LIST_ADD(job,workqueue->waiting_jobs);
    pthread_cond_signal(&workqueue->jobs_cond);
    pthread_mutex_unlock(&workqueue->jobs_mutex);
}