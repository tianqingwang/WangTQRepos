#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "libevent_socket.h"
#include "libevent_multi.h"


static LIBEVENT_THREAD *threads;
struct event_base *main_base;
struct event       main_event;
static last_thread = -1;
static int g_threads_num;

/*free connections*/
static int free_conn_total;
static int free_conn_curr;
static conn **freeconns;

static void (*g_readcb)(int fd);
static void (*g_writecb)(int fd,char *buf);

static void conn_set_state(conn *c,CONN_STATE new_state);
static void conn_close(conn *c);
void on_accept(int fd, short which, void *args);
void on_read(struct bufferevent *bev, void *arg);
void on_write(struct bufferevent *bev,void *arg);

static void cq_init(CQ *cq){
    pthread_mutex_init(&cq->lock,NULL);
    pthread_cond_init(&cq->cond,NULL);
    cq->header = NULL;
    cq->tail   = NULL;
}

static void cq_push(CQ *cq, CQ_ITEM *item){
    item->next = NULL;
    pthread_mutex_lock(&cq->lock);
    if (NULL== cq->tail){
        cq->header = item;
    }
    else{
        cq->tail->next = item;
    }
    cq->tail = item;
    pthread_cond_signal(&cq->cond);
    pthread_mutex_unlock(&cq->lock);
}

static CQ_ITEM *cq_pop(CQ *cq){
    CQ_ITEM *item;
    
    pthread_mutex_lock(&cq->lock);
    item = cq->header;
    if (item != NULL){
        cq->header = item->next;
        if (NULL == cq->header){
            cq->tail = NULL;
        }
    }
    pthread_mutex_unlock(&cq->lock);
    
    return item;
}

static pthread_mutex_t  tlock = PTHREAD_MUTEX_INITIALIZER;


void dispatch_conn(int sfd,CONN_STATE init_state,int ev_flag){
    char buf[1];
    CQ_ITEM *item = malloc(sizeof(CQ_ITEM));
    item->fd = sfd;
    item->state = init_state;
    item->ev_flag = ev_flag;
 
    int tid = (last_thread + 1)%g_threads_num;
    last_thread = tid;
    
    LIBEVENT_THREAD *thread = threads + tid;

    printf("dispatch_conn\n");
    cq_push(&thread->new_conn_queue,item);
    buf[0] = 'c';
    write(thread->notify_send_fd,buf,1);
}


static void create_worker(void *(*func)(void*),void *arg)
{
    LIBEVENT_THREAD *thread = (LIBEVENT_THREAD*)arg;
   
    pthread_attr_t attr;
    int ret ;
    
    pthread_attr_init(&attr);
    
    ret = pthread_create(&thread->thread_id,&attr,func,arg);
    if (ret != 0){
        perror("Failed to create thread.");
        exit(1);
    }
    
}

/*event finite-state machine*/
static void event_FSM(conn *c)
{
    int stop = 0;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connfd;
    int n = 0;
    char rbuf[RBUFSIZE];
    char wbuf[WBUFSIZE];
    
    while(!stop){
        switch(c->state){
            case CONN_STATE_LISTENING:
                printf("CONN_STATE_LISTENING\n");
                connfd = accept(c->fd,(struct sockaddr*)&client_addr,&client_len);
                if (connfd == -1){
                    if (errno == EAGAIN || errno == EWOULDBLOCK){
                        stop = 1;
                    }
                    else if (errno == EMFILE){/*exceed per-process limit of file descriptions.*/
                        fprintf(stderr,"Too many opened connections.\n");
                        stop = 1;
                    }
                    else{
                        fprintf(stderr,"accept error.\n");
                        stop = 1;
                    }
                    break;
                }
                
                if (set_socket_nonblock(connfd) == -1){
                    fprintf(stderr,"Failed to set NONBLOCK.\n");
                    close(connfd);
                    break;
                }

                printf("connected,dispatch_conn.\n");
                dispatch_conn(connfd,CONN_STATE_READ,EV_READ|EV_PERSIST);

                stop = 1;
                
                break;
            case CONN_STATE_READ:
                g_readcb(c->fd);
               
                stop = 1;
                break;
            case CONN_STATE_WRITE:
                
                n = write(c->fd,c->wbuf,strlen(c->wbuf));
                if (n == -1){
                    if (errno == EAGAIN || errno == EWOULDBLOCK){
                        //conn_set_state(c,CONN_STATE_WAIT);
                        break;
                    }
                    else{
                        conn_set_state(c,CONN_STATE_CLOSE);
                        break;
                    }
                }
                else if (n == 0){
                    conn_set_state(c,CONN_STATE_CLOSE);
                    break;
                }
                else{
                    memset(wbuf,0,sizeof(wbuf));
                }
                
                stop = 1;
                break;
            case CONN_STATE_CLOSE:
                conn_close(c);
                stop = 1;
                break;
        }
    }
}

void event_handler(int fd, short which, void *arg)
{
    conn *c = (conn*)arg;
//    printf("event_handler.\n");
    /*event finite-state machine*/
    if (c != NULL){
        if (c->fd != fd){
            conn_close(c);
        }
        event_FSM(c);
    }
    
    return;
}

static pthread_mutex_t conn_lock = PTHREAD_MUTEX_INITIALIZER;

static void conn_init(void)
{
    free_conn_total = 1000;
    free_conn_curr  = 1000;
    freeconns       = malloc(sizeof(conn*)*free_conn_total);
    if (freeconns == NULL){
        fprintf(stderr,"Failed to allocate freeconns.\n");
        exit(1);
    }
    
}

conn* conn_from_freelist(void)
{
	conn *c;
	pthread_mutex_lock(&conn_lock);
    if (free_conn_curr > 0){
    	c = freeconns[--free_conn_curr];
    }
    else{
    	c = NULL;
    }
    pthread_mutex_unlock(&conn_lock);
    
    return c;
}

int conn_add_to_freelist(conn *c)
{
    int ret = -1;
    
    pthread_mutex_lock(&conn_lock);
    
    if (free_conn_curr < free_conn_total){
        freeconns[free_conn_curr++] = c;
        ret = 0;
    }
	else{
        int new_size = free_conn_total*2;
        conn** new_freeconns = realloc(freeconns,new_size*sizeof(conn*));
        if (new_freeconns != NULL){
            free_conn_total = new_size;
            freeconns = new_freeconns;
            freeconns[free_conn_curr++] = c;
            ret = 0;
        }
    }
    
    pthread_mutex_unlock(&conn_lock);
    
    return ret;
}


static conn *conn_new(int fd,CONN_STATE init_state,int ev_flag,struct event_base *base)
{
//    conn *c = malloc(sizeof(conn));
    conn *c = conn_from_freelist();
    
    if (c == NULL){
        c = malloc(sizeof(conn));
        if (c == NULL){
            fprintf(stderr,"malloc error.\n");
            return NULL;
        }
    }

    c->fd = fd;
    c->state = init_state;
    c->ev_flag = ev_flag;
    
    c->wbuf = (char*)malloc(sizeof(char)*WBUFSIZE);
    c->rbuf = (char*)malloc(sizeof(char)*RBUFSIZE);
    c->wsize = WBUFSIZE;
    c->rsize = RBUFSIZE;
    c->rbytes = 0;

    event_set(&c->event,fd,ev_flag,event_handler,(void*)c);
    event_base_set(base,&c->event);
    event_add(&c->event,0);

    return c;
}

static void conn_set_state(conn *c,CONN_STATE new_state)
{
    if (c != NULL && (new_state >=CONN_STATE_LISTENING && new_state <= CONN_STATE_CLOSE)){
        if (c->state != new_state){
            c->state = new_state;
        }
    }
}

static void conn_free(conn *c)
{
    if (c != NULL){
        if (conn_add_to_freelist(c) == -1){
            if (c->rbuf){
                free(c->rbuf);
                c->rbuf = NULL;
                c->rsize = 0;
            }
            
            if (c->wbuf){
                free(c->wbuf);
                c->wbuf = NULL;
                c->wsize = 0;
            }
            
            free(c);
        }
    }
}

static void conn_close(conn *c)
{   
    if (c != NULL){
        /*close socket*/
        close(c->fd);
        
        /*clean data */
        c->fd = -1;
        c->state = -1;
        c->ev_flag = -1;
        event_del(&c->event);
        
        /*free the allocated memory.*/
        conn_free(c);
    }
}

static void thread_libevent_process(int fd, short which,void *arg)
{
    LIBEVENT_THREAD *this = arg;
    CQ_ITEM *item;
    
    char buf[1];
    if (read(fd,buf,1) != 1){
        perror("can't read from libevent pipe.");
    }
    printf("thread_libevent_process\n");
    item = cq_pop(&this->new_conn_queue);

    struct bufferevent *bev = bufferevent_socket_new(this->base,item->fd,BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev,on_read,on_write,NULL,NULL);
    bufferevent_enable(bev,EV_READ|EV_WRITE|EV_PERSIST);
    
    free(item);
}

static void *worker_libevent(void *arg)
{
    LIBEVENT_THREAD *me = arg;
    printf("worker_libevent\n");
    
    event_base_loop(me->base,0);
    
    return NULL;
}


int worker_thread_init(int nthreads)
{
    int i;
    
    if (nthreads <= 0){
        g_threads_num = DEFAULT_MAXTHREADS;
    }
    else{
        g_threads_num = nthreads;
    }
    
    threads =(LIBEVENT_THREAD*) malloc(sizeof(LIBEVENT_THREAD)*nthreads);
    if (threads == NULL){
        return -1;
    }
    
    for (i=0; i<nthreads; i++){
        int fds[2];
        if (pipe(fds)){
            perror("Failed to create pipe.");
            return -1;
        }
        threads[i].notify_receive_fd = fds[0];
        threads[i].notify_send_fd    = fds[1];
        
        setup_thread(&threads[i]);
    }
    
    /*create worker threads.*/
    for (i=0; i<nthreads; i++){
        create_worker(worker_libevent,&threads[i]);
    }
}


int setup_thread(LIBEVENT_THREAD *thread)
{
    thread->base = event_init();
    if (thread->base == NULL){
        perror("Failed to allocate event base.");
        return -1;
    }
    
    event_set(&thread->notify_event,thread->notify_receive_fd,EV_READ|EV_PERSIST,thread_libevent_process,thread);
    event_base_set(thread->base,&thread->notify_event);
    if (event_add(&thread->notify_event,0) == -1){
        perror("Failed to add event.");
        return -1;
    }
    
    cq_init(&thread->new_conn_queue);
}

void master_thread_loop(int sockfd)
{
    main_base = event_init();
    
    conn_init();

#if 0    
    /*setup main_base in charge of accept connection.*/
    conn *c = conn_new(sockfd,CONN_STATE_LISTENING,EV_READ|EV_PERSIST,main_base);
#else
    conn *c = conn_from_freelist();
    
    if (c == NULL){
        c = malloc(sizeof(conn));
        if (c == NULL){
            fprintf(stderr,"malloc error.\n");
            return ;
        }
    }

    c->fd = sockfd;
    c->state = CONN_STATE_LISTENING;
    c->ev_flag = EV_READ|EV_PERSIST;
    
    c->wbuf = (char*)malloc(sizeof(char)*WBUFSIZE);
    c->rbuf = (char*)malloc(sizeof(char)*RBUFSIZE);
    c->wsize = WBUFSIZE;
    c->rsize = RBUFSIZE;
    c->rbytes = 0;
    event_set(&main_event,sockfd,EV_READ|EV_PERSIST,on_accept,(void*)c);
    event_base_set(main_base,&main_event);
    event_add(&main_event,0);
#endif    
    event_base_loop(main_base,0);
}

void set_read_callback(void (*callback)(int))
{
    g_readcb = callback;
}

void set_write_callback(void (*callback)(int,char*))
{
    g_writecb = callback;
}

/*main_base is in charge of accept.*/
void on_accept(int fd, short which, void *args)
{
    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    conn *c = (conn*)args;
    
    connfd = accept(c->fd,(struct sockaddr*)&client_addr,&client_len);
    if (connfd == -1){
        if (errno == EAGAIN || errno == EWOULDBLOCK){
            
        }
        else{
            fprintf(stderr,"Too many open connections.\n");
            return;
        }
    }
    
    if (set_socket_nonblock(connfd) == -1){
        fprintf(stderr,"failed to set NONBLOCK.\n");
        close(connfd);
    }
    
    printf("thread_id=0x%x,connfd = %d\n",pthread_self(),connfd);
    
    
    dispatch_conn(connfd,CONN_STATE_READ,EV_READ|EV_PERSIST);
    
}


void on_read(struct bufferevent *bev, void *arg)
{
    int n;
    char line[1024]={0};
    evutil_socket_t fd = bufferevent_getfd(bev);
    
    while((n=bufferevent_read(bev,line,1024)) > 0){
        line[n] = '\0';
        printf("fd=%d,pthread_id=0x%x,server received from on_read: %s\n",fd,pthread_self(),line);
    }
}

void on_write(struct bufferevent *bev,void *arg)
{
    
}







