#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "threadpool.h"

CThreadPool::CThreadPool(int maxThreads)
{
    m_maxThreads = maxThreads;
    m_curThreads = 0;

    m_thread_pool.shutdown = 0;
    
    pthread_attr_init(&m_attr);
    set_thread_attr(PTHREAD_CREATE_DETACHED, PTHREAD_SCOPE_SYSTEM);
    pthread_mutex_init(&m_thread_pool.pool_lock,NULL);
    
    create_thread_pool();
}


CThreadPool::~CThreadPool()
{
    int i;

    for (i=0; i<m_curThreads; i++){
        m_thread_pool.thread_info[i].stopflag = 1;

        /*wait thread to be over*/
        while(pthread_kill(m_thread_pool.thread_info[i].thread_id,0) != ESRCH){
        }
    }

    m_thread_pool.shutdown = 1;
    
    delete [] m_thread_pool.thread_info;
    m_thread_pool.thread_info = NULL;

    m_maxThreads = 0;
    m_curThreads = 0;
    
    pthread_mutex_destroy(&m_thread_pool.pool_lock);
}


void CThreadPool::create_thread_pool()
{
    int i=0;
    m_thread_pool.thread_info = NULL;

    m_thread_pool.thread_info = new thread_info_t[m_maxThreads];
     
    for (i=0; i<m_maxThreads; i++){
        m_thread_pool.thread_info[i].proc = NULL;
        m_thread_pool.thread_info[i].args = NULL;
      
        pthread_create(&m_thread_pool.thread_info[i].thread_id,&m_attr,&CThreadPool::worker_thread,(void*)this);
    }

}

void CThreadPool::set_thread_attr(int detached,int scope)
{
    m_detached = detached;
    m_scope    = scope;
    
    pthread_attr_setdetachstate(&m_attr,m_detached);
    pthread_attr_setscope(&m_attr,m_scope);

}

void *CThreadPool::worker_thread(void *args)
{
    int work_thread_id;
    int index;
   
    CThreadPool *pTP = (CThreadPool*)args; 
   
    work_thread_id = pthread_self();
    index = pTP->get_thread_by_id(work_thread_id);
    

    while(!pTP->m_thread_pool.thread_info[index].stopflag)
    {
        if (pTP->m_thread_pool.thread_info[index].proc != NULL){
            pTP->m_thread_pool.thread_info[index].proc(pTP->m_thread_pool.thread_info[index].args);
        }
    }
}


int CThreadPool::get_thread_by_id(int id)
{
    int i;
    
    for (i=0; i<m_curThreads; i++){
        if (id == m_thread_pool.thread_info[i].thread_id){
            return i;
        }
    }

    return -1;
}



void CThreadPool::register_task(process_callback func, void *args)
{
    int i;
    
    m_curThreads ++;

    for (i=0; i<m_curThreads; i++){
        if (m_thread_pool.thread_info[i].proc == NULL){
            m_thread_pool.thread_info[i].proc = func;
            m_thread_pool.thread_info[i].args = args;
            m_thread_pool.thread_info[i].stopflag = 0; 
            break;
        }
    }
}
