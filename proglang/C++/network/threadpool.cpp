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

#if 0
void *CThreadPool::manage_thread(void *args)
{
    
    sleep(MANAGE_INTERVAL);
    CThreadPool *pTP = (CThreadPool*)args;
    
    do{
        if (pTP->get_pool_status() == 0){
            do{
                if (pTP->del_thread() == 0){
                    break;
                }
            }while(1);
        }
        sleep(MANAGE_INTERVAL);
    }while(!pTP->m_thread_pool.shutdown);
}


int CThreadPool::get_pool_status()
{
    float busy_num = 0.0;
    int i = 0;
 
    for (i=0; i<m_curThreads; i++){
        if (m_thread_pool.thread_info[i].is_busy){
            busy_num ++;
        }
    }

    if ((busy_num/m_curThreads) < BUSY_THRESHOLD){
        return 0;
    }
    
    return 1;
}
#endif

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


#if 0
bool CThreadPool::add_thread()
{
    if (m_curThreads >= m_maxThreads){
        return false;
    }

    printf("add new thread\n");    
    

    thread_info_t *new_thread= &m_thread_pool.thread_info[m_curThreads];
    
    if (new_thread == NULL){
        return false;
    }
    
    pthread_mutex_lock(&m_thread_pool.pool_lock);
    m_curThreads ++;
    pthread_mutex_unlock(&m_thread_pool.pool_lock);
    
    pthread_create(&new_thread->thread_id,&m_attr, &CThreadPool::worker_thread,(void*)this);
    
    new_thread->is_busy = true;
}

bool CThreadPool::del_thread()
{
    if (m_curThreads <= m_minThreads){
        return false;
    }

    if (m_thread_pool.thread_info[m_curThreads-1].is_busy){
        return false; 
    }
    
    pthread_mutex_lock(&m_thread_pool.pool_lock); 
    m_curThreads--;
    pthread_mutex_unlock(&m_thread_pool.pool_lock);
    
    kill(m_thread_pool.thread_info[m_curThreads].thread_id, SIGKILL);

}

#endif

#if 0
void CThreadPool::pool_close()
{   
    int i = 0;

    if (m_detached == PTHREAD_CREATE_JOINABLE){
        
        m_thread_pool.shutdown = 1;
        
        for (i=0; i<m_curThreads; i++){
            pthread_join(m_thread_pool.thread_info[i].thread_id,NULL);
        }
    } 
    else{
        /*detached threads*/
        m_thread_pool.shutdown = 1;
    }
}
#endif

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
