/****************************************************
* Multi-threads work queue.
* Author: Wang Tianqing
* Date  : 2013-09-18
*****************************************************/
#include <unistd.h>
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

#define MAX_JOB_NUM    (1024)

static workqueue_t *pool = NULL;

/*variables for free jobs list*/
static job_t         **freejobslist;
static int             freejobslist_total;
static int             freejobslist_cur;
static pthread_mutex_t freejobslist_mutex;
static pthread_cond_t  freejobslist_cond;

/*functions for free jobs list*/
static void   init_freejobslist(int njobs);
static job_t *get_from_freejobslist();
static void   add_to_freejobslist(job_t *job);
static void   free_freejobslist();

/*variables for user_data_t*/
static user_data_t     **freedatalist;
static int               freedatalist_total;
static int               freedatalist_cur;
static pthread_mutex_t   freedatalist_mutex;
static pthread_cond_t    freedatalist_cond;

static void *worker_function(void *args)
{
    
    while(1){
        pthread_mutex_lock(&(pool->jobs_mutex));
        while(pool->cur_queue_size == 0 && !pool->shutdown){
            pthread_cond_wait(&(pool->jobs_cond),&(pool->jobs_mutex));
        }
        
        if (pool->shutdown){
            pthread_mutex_unlock(&(pool->jobs_mutex));
            break;
        }
        
        pool->cur_queue_size --;
        job_t *job = pool->jobs_head;
        pool->jobs_head = job->next;
       
        pthread_mutex_unlock(&(pool->jobs_mutex));
        
        /*to avoid segment fault.*/
        if (job == NULL) continue;
        if (job->job_function == NULL){
            add_to_freejobslist(job);
            continue;
        }
        
        job->job_function(job->arg);
        
        /*now job is free again.*/
        add_to_freejobslist(job);
    }
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
    
    /*pre-allocate jobs.*/
    init_freejobslist(MAX_JOB_NUM);
    
    pool->threadid = (pthread_t*)malloc(nworks*sizeof(pthread_t));
    if (pool->threadid == NULL){
        fprintf(stderr,"failed to create thread pool.\n");
        exit(EXIT_FAILURE);
    }
    int i = 0;
    for (i=0; i<nworks; i++){
        if (pthread_create(&(pool->threadid[i]),NULL,worker_function,NULL) < 0){
            fprintf(stderr,"pthread_create failed.\n");
            exit(EXIT_FAILURE);
        }
    }
}


/*purpose: shutdown thread pool, free workqueue*/
void  workqueue_shutdown()
{
    if (pool->shutdown){
        return;
    }
    pool->shutdown = 1;
    
    /*wake up all blocked thread.*/
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
        head->next = NULL;
        add_to_freejobslist(head);
    }
    
    free_freejobslist();
    free(pool);
    pool = NULL;
}

/*purpose: add a new job to workqueue*/
void workqueue_add_job(void *(*job_function)(void *arg),void *arg)
{
    job_t *job = get_from_freejobslist();
    
    if (job == NULL) return;
    
    job->job_function = job_function;
    job->arg = arg;
    job->next = NULL;
    
    pthread_mutex_lock(&(pool->jobs_mutex));
    /*add to job queue.*/
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
}

/*pre-allocate memory for job list.*/
static void init_freejobslist(int njobs)
{
    int i;
    
    freejobslist = malloc(sizeof(job_t*)*njobs);
    if (freejobslist == NULL){
        fprintf(stderr,"failed to allocate memory for freejobslist.\n");
        exit(EXIT_FAILURE);
    }
    
    for (i=0; i<njobs; i++){
        freejobslist[i] = malloc(sizeof(job_t));
        if (freejobslist[i] == NULL){
            fprintf(stderr,"failed to allocate memory for freejobslist.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    freejobslist_total = njobs;
    freejobslist_cur   = njobs;
    
    pthread_mutex_init(&freejobslist_mutex,NULL);
    pthread_cond_init(&freejobslist_cond,NULL);
}

/*add one unused job to free job list.*/
static void add_to_freejobslist(job_t *job)
{
    pthread_mutex_lock(&freejobslist_mutex);
    
    if (freejobslist_cur < freejobslist_total){
        freejobslist[freejobslist_cur++] = job;
    }
    pthread_cond_signal(&freejobslist_cond);
    
    pthread_mutex_unlock(&freejobslist_mutex);
}

/*get a job from free job list.*/
static job_t *get_from_freejobslist()
{
    job_t *job;
    
    pthread_mutex_lock(&freejobslist_mutex);
    
    while(freejobslist_cur == 0){
        /*now freejobslist is unavailable, block and wait for signal 
         *until list is available again.
         */
        pthread_cond_wait(&freejobslist_cond,&freejobslist_mutex);
    }
    
    if (freejobslist_cur > 0){
        job = freejobslist[--freejobslist_cur];
    }
    
    pthread_mutex_unlock(&freejobslist_mutex);
    
    return job;
}

/*free all allocated memory for job list.*/
static void free_freejobslist()
{
    int i;
    
    for (i=0; i<freejobslist_total; i++){
        free(freejobslist[i]);
    }
    
    free(freejobslist);
    
    pthread_mutex_destroy(&freejobslist_mutex);
    pthread_cond_destroy(&freejobslist_cond);
}

void init_datalist(int nMaxConnection)
{
    int i;
    
    freedatalist = malloc(sizeof(user_data_t*)*nMaxConnection);
    if (freedatalist == NULL){
        fprintf(stderr,"failed to allocate memory for datalist.\n");
        exit(EXIT_FAILURE);
    }
    
    for (i=0; i<nMaxConnection; i++){
        freedatalist[i] = malloc(sizeof(user_data_t));
        if (freedatalist[i] == NULL){
            fprintf(stderr,"failed to allocated memory for datalist.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    freedatalist_total = nMaxConnection;
    freedatalist_cur   = nMaxConnection;
    
    pthread_mutex_init(&freedatalist_mutex,NULL);
    pthread_cond_init(&freedatalist_cond,NULL);
}

user_data_t *get_from_datalist()
{
    user_data_t *user_data;
    
    pthread_mutex_lock(&freedatalist_mutex);
    
    while(freedatalist_cur == 0){
        pthread_cond_wait(&freedatalist_cond,&freedatalist_mutex);
    }
    
    if (freedatalist_cur > 0){
        user_data = freedatalist[--freedatalist_cur];
    }
    
    pthread_mutex_unlock(&freedatalist_mutex);
    
    return user_data;
}

void add_to_datalist(user_data_t *user_data)
{
    pthread_mutex_lock(&freedatalist_mutex);
    
    if (freedatalist_cur < freedatalist_total){
        freedatalist[freedatalist_cur++] = user_data;
    }
    
    pthread_cond_signal(&freedatalist_cond);
    pthread_mutex_unlock(&freedatalist_mutex);
}

 void free_datalist()
{
    int i;
    
    for (i=0; i<freedatalist_total; i++){
        free(freedatalist[i]);
    }
    
    free(freedatalist);

    pthread_mutex_destroy(&freedatalist_mutex);
    pthread_cond_destroy(&freedatalist_cond);
}
