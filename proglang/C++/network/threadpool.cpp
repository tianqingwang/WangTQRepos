#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include "threadpool.h"

CThreadPool::CThreadPool()
{
    m_minThreads = 0;
    m_maxThreads = 0;
    m_curThreads = 0;
    
    pthread_attr_init(&m_attr);
    /*set the default attribute of the thread*/
    m_detached = PTHREAD_CREATE_DETACHED;
    m_scope    = PTHREAD_SCOPE_SYSTEM;
    
    m_thread_pool.shutdown = 0;
    
    pthread_attr_setdetachstate(&m_attr,m_detached);
    pthread_attr_setscope(&m_attr,m_scope);
}

CThreadPool::~CThreadPool()
{
    pthread_attr_destroy(&m_attr);
    DestroyThreadPool();
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
bool CThreadPool::CreateThreadPool(int minThreads, int maxThreads)
{
    int i;
    int err;
    
    memset(&m_thread_pool,0,sizeof(thread_pool_t));
    
    m_minThreads = minThreads; 
    m_maxThreads = maxThreads;
    m_curThreads = m_minThreads;
    /*pool lock initialize*/
    pthread_mutex_init(&m_thread_pool.pool_lock,NULL);
    m_thread_pool.shutdown = 0; /*not shutdown*/
    
    if (NULL != m_thread_pool.thread_info){
        delete [] m_thread_pool.thread_info;
    }
    m_thread_pool.thread_info = new thread_info_t[m_maxThreads];
    
    /*create worker threads*/
    for (i=0; i<m_minThreads; i++){
        m_thread_pool.thread_info[i].process_func = NULL;
        m_thread_pool.thread_info[i].args         = NULL;
        err = pthread_create(&m_thread_pool.thread_info[i].thread_id,&m_attr,work_thread,NULL);
        if (err != 0){
            return false;
        }
    }
    
    /*create manage thread*/
    err = pthread_create(&m_ManageThreadID,&m_attr,manage_thread,NULL);
    if (err != 0){
        return false;
    }
    
    return true;
}

void CThreadPool::RegisterProc(void *func,void *args)
{
    int i;
    
    for (i=0; i<m_curThreads; i++){
        if (m_thread_pool.thread_info[i].process_func == NULL){
            m_thread_pool.thread_info[i].process_func = func;
            m_thread_pool.thread_info[i].args         = args;
            m_thread_pool.thread_info[i].is_busy      = true;
            break;
        }
    }
}

void *CThreadPool::work_thread(void *arg)
{
    int work_thread_id;
    int idx;
    
    work_thread_id = pthread_self();
    idx = GetThreadByID(work_thread_id);
    if (idx == -1){
        return NULL;
    }
    
    while(!m_thread_pool.shutdown){
        pthread_mutex_lock(&m_thread_pool.thread_info[idx].thread_lock);
        pthread_cond_wait(&m_thread_pool.thread_info[idx].thread_cond,&m_thread_pool.thread_info[idx].thread_lock);
        pthread_mutex_unlock(&m_thread_pool.thread_info[idx].thread_lock);
        
        if (m_thread_pool.thread_info[idx].process_func){
            m_thread_pool.thread_info[idx].process_func(m_thread_pool.thread_info[idx].args);
        }
        
        pthread_mutex_lock(&m_thread_pool.thread_info[idx].thread_lock);
        m_thread_pool.thread_info[idx].is_busy = false;
        pthread_mutex_unlock(&m_thread_pool.thread_info[idx].thread_lock);
    }
    
}

void *CThreadPool::manage_thread(void *arg)
{
    sleep(MANAGE_INTERVAL);
    
    do{
        if (GetPoolStatus() == 0){/*idle*/
            do{
                if (DeleteThread() == 0){
                    break;
                }
            }while(1);
        }
        sleep(MANAGE_INTERVAL);
    }while(!m_thread_pool.shutdown);
}

bool CThreadPool::AddNewThread()
{
    int err;
    thread_info_t *new_thread;
    pthread_mutex_lock(&m_thread_pool.pool_lock);
    if (m_curThreads >= m_maxThreads){
        pthread_mutex_unlock(&m_thread_pool.pool_lock);
        return false;
    }
    
    new_thread = &m_thread_pool.thread_info[m_curThreads];
    m_curThreads ++;
    pthread_mutex_unlock(&m_thread_pool.pool_lock);
    
    new_thread->process_func  = NULL;
    new_thread->args          = NULL;
    err = pthread_create(&new_thread->thread_id,&m_attr,&work_thread,NULL);
    if (err != 0){
        return false;
    }
    
    pthread_cond_init(&new_thread->thread_cond,NULL);
    pthread_mutex_init(&new_thread->thread_lock,NULL);
    
    new_thread->is_busy = true;
    
    return true;
    
}

bool CThreadPool::DeleteThread()
{
    if (m_curThreads <= m_minThreads){
        return false;
    }
    
    if (m_thread_pool.thread_info[m_curThreads-1].is_busy == true){
        return false;
    }
    
    pthread_mutex_lock(&m_thread_pool.pool_lock);
    m_curThreads--;
    pthread_mutex_unlock(&m_thread_pool.pool_lock);
    
    kill(m_thread_pool.thread_info[m_curThreads].thread_id,SIGKILL);
    pthread_mutex_destroy(&m_thread_pool.thread_info[m_curThreads].thread_lock);
    pthread_cond_destroy(&m_thread_pool.thread_info[m_curThreads].thread_cond);

    return true;    
}

void CThreadPool::DestroyThreadPool()
{
    int i;
    
    /*close work threads*/
    for (i=0; i<m_curThreads; i++){
        kill(m_thread_pool.thread_info[i].thread_id,SIGKILL);
        pthread_mutex_destroy(&m_thread_pool.thread_info[i].thread_lock);
        pthread_cond_destroy(&m_thread_pool.thread_info[i].thread_cond);
    }
    
    /*kill manage thread*/
    kill(m_ManageThreadID,SIGKILL);
    pthread_mutex_destroy(&m_thread_pool.pool_lock);
    
    delete [] m_thread_pool.thread_info;
    
    m_minThreads = 0;
    m_maxThreads = 0;
    m_curThreads = 0;
}

int CThreadPool::GetPoolStatus()
{
    float busy_num = 0.0;
    int i;
    
    for (i=0; i<m_curThreads; i++){
        if (m_thread_pool.thread_info[i].is_busy){
            busy_num ++;
        }
    }
    
    if ((busy_num/m_curThreads) < BUSY_THRESHOLD){
        /*idle*/
        return 0;
    }
    else{
        /*busy*/
        return 1;
    }
}

int CThreadPool::GetThreadByID(int id)
{
    int i;
    
    for (i=0; i<m_curThreads; i++){
        if (id == m_thread_pool.thread_info[i].thread_id){
            return i;
        }
    }
    
    return -1;
}
