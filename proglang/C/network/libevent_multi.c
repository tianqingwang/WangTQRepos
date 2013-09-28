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
static last_thread = -1;
static int g_threads_num;

static void conn_set_state(conn *c,CONN_STATE new_state);
static void conn_close(conn *c);

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


void dispatch_conn(int sfd,CONN_STATE init_state,int ev_flag){
    char buf[1];
    CQ_ITEM *item = malloc(sizeof(CQ_ITEM));
    item->fd = sfd;
    item->state = init_state;
    item->ev_flag = ev_flag;
    
    int tid = (last_thread + 1)%g_threads_num;
    last_thread = tid;
    
    LIBEVENT_THREAD *thread = threads + tid;
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
                
                dispatch_conn(connfd,CONN_STATE_READ,EV_READ|EV_PERSIST);
                
                stop = 1;
                
                break;
            case CONN_STATE_READ:
                n = read(c->fd,rbuf,RBUFSIZE);
                if (n == -1){
                    if (errno == EAGAIN || errno == EWOULDBLOCK){
                        //conn_set_state(c,CONN_STATE_WAIT);
                        break;
                    }
                    else{
                        /*read error, maybe connection dropped.*/
                        conn_set_state(c,CONN_STATE_CLOSE);
                        break;
                    }
                }
                else if(n == 0){
                    conn_set_state(c,CONN_STATE_CLOSE);
                    break;
                }
                else{
                    /*got data*/
                    /*fixme: if n==RBUFSIZE*/
                    rbuf[n] = '\0';
                    fprintf(stdout,"server received:%s\n",rbuf);
                    memset(rbuf,0,sizeof(rbuf));
                }
                /*for test*/
                dispatch_conn(c->fd,CONN_STATE_WRITE,EV_WRITE);
                stop = 1;
                break;
            case CONN_STATE_WRITE:
                strcpy(wbuf,"hi,I'm server.");
                n = write(c->fd,wbuf,strlen(wbuf));
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
    
    /*event finite-state machine*/
    if (c != NULL){
        event_FSM(c);
    }
    
    return;
}


static conn *conn_new(int fd,CONN_STATE init_state,int ev_flag,struct event_base *base)
{
    conn *c = malloc(sizeof(conn));

    c->fd = fd;
    c->base = base;
    c->state = init_state;

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
        free(c);
    }
}

static void conn_close(conn *c)
{   
    if (c != NULL){
        /*delete event.*/
        event_del(&c->event);
        
        /*close socket*/
        close(c->fd);
        
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
    
    item = cq_pop(&this->new_conn_queue);
    
    if (item != NULL){
        conn *c = conn_new(item->fd,item->state,item->ev_flag,this->base);
        if (c == NULL){
            close(item->fd);
        }
    }
    
    free(item);
}

static void *worker_libevent(void *arg)
{
    LIBEVENT_THREAD *me = arg;
    
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
    
    /*setup main_base in charge of accept connection.*/
    conn *c = conn_new(sockfd,CONN_STATE_LISTENING,EV_READ|EV_PERSIST,main_base);
    
    event_base_loop(main_base,0);
}



