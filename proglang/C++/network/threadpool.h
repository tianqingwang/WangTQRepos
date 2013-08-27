#include <pthread.h>
#include <string.h>


typedef void (*process_callback)(void *args);

typedef struct thread_info_s{
    pthread_t          thread_id;
    process_callback   proc;
    void               *args;
    int                stopflag;
}thread_info_t;

typedef struct thread_pool_s{
    thread_info_t   *thread_info;
    pthread_mutex_t pool_lock;
   
    int shutdown;
}thread_pool_t;


class CThreadPool{
public:
    
    CThreadPool(int maxThreads);
    ~CThreadPool();

    void create_thread_pool();
    void set_thread_attr(int detached,int scope);
    int  get_thread_by_id(int id);
    static void *worker_thread(void *args);

    void  register_task(process_callback Func,void *args);
private:
    int m_maxThreads;
    int m_curThreads;

    pthread_attr_t m_attr;
    int            m_detached;
    int            m_scope;

    thread_pool_t  m_thread_pool;
};
