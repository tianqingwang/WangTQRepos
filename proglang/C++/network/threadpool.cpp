#include "threadpool.h"

CThreadPool::CThreadPool()
{
    m_minThreads = 1;
    m_maxThreads = 1;
    m_curThreads = 1;
    
    pthread_attr_init(&m_attr);
    /*set the default attribute of the thread*/
    m_detached = PTHREAD_CREATE_DETACHED;
    m_scope    = PTHREAD_SCOPE_SYSTEM;
    
    pthread_attr_setdetachstate(&m_attr,m_detached);
    pthread_attr_setscope(&m_attr,m_scope);
}

CThreadPool::~CThreadPool()
{
    pthread_attr_destroy(&m_attr);
}

/**********************************************
**Purpose: set detach state and scope
**int detached: PTHREAD_CREATE_DETACHED(default)
**              PTHREAD_CREATE_JOINABLE
**int scope   : PTHREAD_SCOPE_SYSTEM(default) 
**              PTHREAD_SCOPE_PROCESS
***********************************************/
void CThreadPool::SetThreadAttr(int detached, int scope)
{
    m_detached = detached;
    m_scope    = scope;
    pthread_attr_setdetachstate(&m_attr,m_detached);
    pthread_attr_setscope(&m_attr,m_scope);
}

/*****************************************
**create at least minThreads worker threads.
** and a management thread.
**int minThreads: at least threads
**int maxThreads: the maximum threads
******************************************/
thread_pool_t *CreateThreadPool(int minThreads, int maxThreads)
{
    int i;
    int err;
    
    thread_pool_t *thread_pool = new thread_pool_t;
    memset(thread_pool,0,sizeof(thread_pool_t));
    
    m_minThreads = minThreads; 
    m_maxThreads = maxThreads;
    m_curThreads = m_minThreads;
    /*pool lock initialize*/
    pthread_mutex_init(thread_pool->tp_lock,NULL);
    thread_pool->shutdown = 0; /*not shutdown*/
    
    if (NULL != thread_pool->thread_info){
        delete [] thread_pool->thread_info;
    }
    thread_pool->thread_info = new thread_info_t[m_maxThreads];
    
    /*create worker threads*/
    for (i=0; i<m_minThreads; i++){
        err = pthread_create(&thread_pool->thread_info[i].thread_id,&m_attr,
    }
    /*create manage thread*/
    
    
    return thread_pool;
}
