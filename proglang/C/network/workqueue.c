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


static workqueue_t *pool = NULL;

static void *worker_function(void *args)
{
    
    while(1){
        pthread_mutex_lock(&(pool->jobs_mutex));
        while(pool->cur_queue_size == 0 && !pool->shutdown){
            pthread_cond_wait(&(pool->jobs_cond),&(pool->jobs_mutex));
        }
        
        if (pool->shutdown){
            pthread_mutex_unlock(&(pool->jobs_mutex));
            
            pthread_exit(NULL);
        }
        
        pool->cur_queue_size --;
        job_t *job = pool->jobs_head;
        pool->jobs_head = job->next;
       
        pthread_mutex_unlock(&(pool->jobs_mutex));
        
        if (job == NULL) continue;
        job->job_function(job->arg);
        
        free(job);
        job = NULL;
    }
    pthread_exit(NULL);
}

/*purpose: create thread pool to run the jobs in workqueue*/
int  workqueue_init(int nworks)
{
    pool = (workqueue_t*)malloc(sizeof(workqueue_t));
    pthread_mutex_init(&(pool->jobs_mutex),NULL);
    pthread_cond_init(&(pool->jobs_cond),NULL);
    
    pool->jobs_head = NULL;
    pool->jobs_tail = NULL;
    pool->max_thread_num = nworks;
    pool->cur_queue_size = 0;
    
    pool->shutdown = 0;
    
    pool->threadid = (pthread_t*)malloc(nworks*sizeof(pthread_t));
    int i = 0;
    for (i=0; i<nworks; i++){
        pthread_create(&(pool->threadid[i]),NULL,worker_function,NULL);
    }
}


/*purpose: shutdown thread pool, free workqueue*/
void  workqueue_shutdown()
{
    if (pool->shutdown){
        return;
    }
    pool->shutdown = 1;
    
    pthread_cond_broadcast(&(pool->jobs_cond));
    
    int i;
    for (i=0; i<pool->max_thread_num; i++){
        pthread_join(pool->threadid[i],NULL);
    }
    free(pool->threadid);
    
    job_t *head = NULL;
    while(pool->jobs_head != NULL){
        head = pool->jobs_head;
        pool->jobs_head = pool->jobs_head->next;
        free(head);
    }
    
    free(pool);
    pool = NULL;
}

/*purpose: add a new job to workqueue*/
void workqueue_add_job(void *(*job_function)(void *arg),void *arg)
{
    job_t *job = (job_t*)malloc(sizeof(job_t));
    job->job_function = job_function;
    job->arg = arg;
    job->next = NULL;
    
    pthread_mutex_lock(&(pool->jobs_mutex));
    
    if (pool->jobs_head != NULL){
        pool->jobs_tail->next = job;
        pool->jobs_tail = job;
    }
    else{
        pool->jobs_head = job;
        pool->jobs_tail = pool->jobs_head;
    }
    pool->cur_queue_size ++;
    
    pthread_cond_signal(&(pool->jobs_cond));
    pthread_mutex_unlock(&(pool->jobs_mutex));
    
//    return 0;
}