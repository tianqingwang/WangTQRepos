#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__
#include <pthread.h>
#include <sys/types.h>

#define BUSY_THRESHOLD   0.5
#define MANAGE_INTERVAL  5

typedef void (*process_job)(void* args);

typedef struct thread_info_s{
    pthread_t            thread_id;
    bool                 is_busy;
    pthread_cond_t       thread_cond;
    pthread_mutex_t      thread_lock;
    process_job          process_func;
    void                 *args;
    
}thread_info_t;

typedef struct thread_pool_s{
    int                  shutdown;
    pthread_mutex_t      pool_lock;
    thread_info_t        *thread_info;
}thread_pool_t;

class CThreadPool{
public:
    /*constructor and de-constructor*/
    CThreadPool();
    ~CThreadPool();
    
    void          SetThreadAttr(int detached=PTHREAD_CREATE_DETACHED, int scope=PTHREAD_SCOPE_SYSTEM);
    bool          CreateThreadPool(int minThreads, int maxThreads);
    void          RegisterProc(void *func,void *args); /*register processing function*/
    void   *work_thread(void *arg);
    void   *manage_thread(void *arg);
    void          DestroyThreadPool();
    
    bool          AddNewThread();
    bool          DeleteThread();
    int           GetPoolStatus();
    int           GetThreadByID(int id);
private:
    pthread_t m_ManageThreadID;
    
    thread_pool_t m_thread_pool;
    
    int m_minThreads; /*at least*/
    int m_maxThreads; /*maximum threads*/
    int m_curThreads; /*how many threads in pool now*/
    
    /*thread attributes*/
    pthread_attr_t m_attr; /**/
    int            m_detached; /*JOIN or DETACH, default is JOIN*/
    int            m_scope;    /*PROCESS or SYSTEM, default is SYSTEM*/
    
    
};

#endif
