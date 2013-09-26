#ifndef __LIBEVENT_MULTI_H__
#define __LIBEVENT_MULTI_H__
#include <event.h>
#include <pthread.h>

#define DEFAULT_MAXTHREADS 2
#define RBUFSIZE           1024
#define WBUFSIZE           1024

typedef enum conn_state{
    CONN_STATE_LISTENING = 0x0,
    CONN_STATE_READ,
    CONN_STATE_WRITE,
    CONN_STATE_WAITING,
    CONN_STATE_CLOSE
}CONN_STATE;

typedef struct cq_item{
    int fd;
    CONN_STATE state;
    int ev_flag;
    struct cq_item *next;
}CQ_ITEM;

typedef struct cq{
    CQ_ITEM *header;
    CQ_ITEM *tail;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
}CQ;

typedef struct _LIBEVENT_THREAD{
    pthread_t thread_id;
    int notify_receive_fd;
    int notify_send_fd;
    struct event_base *base;
    struct event notify_event;
    CQ     new_conn_queue;
}LIBEVENT_THREAD;

typedef struct _CONNECT{
    int fd;
    CONN_STATE state;
    struct event_base *base;
    struct event event;
}conn;

int worker_thread_init(int nthreads);
void master_thread_loop(int sockfd);

#endif
