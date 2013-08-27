#include <pthread.h>
#include <string.h>

#define BUSY_THRESHOLD   0.5
#define MANAGE_INTERVAL  5

typedef void (*process_callback)(void *args);

typedef struct thread_info_s{
    pthread_t          thread_id;
    bool               is_busy;
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
    
    CThreadPool();
    CThreadPool(int minThreads,int maxThreads);
    ~CThreadPool();
    
    void destroy_pool();    

    void create_thread_pool();
    void set_thread_attr(int detached,int scope);

    static void *worker_thread(void *args);
    static void *manage_thread(void *args);

    int   get_pool_status();
    int   get_thread_by_id(int id);
    bool  add_thread();
    bool  del_thread();
    void  pool_close();
    void  register_task(process_callback Func,void *args);
private:
    int m_minThreads;
    int m_maxThreads;
    int m_curThreads;

    pthread_attr_t m_attr;
    int            m_detached;
    int            m_scope;

    thread_pool_t  m_thread_pool;
    pthread_t      m_manage_id;
};
