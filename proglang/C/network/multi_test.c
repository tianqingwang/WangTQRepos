#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <event.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

#define NPORT   5000
#define BACKLOG  5
#define MAXTHREADS 3
#define RBUFSIZE   (1024)
#define WBUFSIZE   (1024)

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


static LIBEVENT_THREAD *threads;
struct event_base *main_base;


static conn *conn_new(int fd,CONN_STATE init_state,struct event_base *base);
static void conn_set_state(conn *c,CONN_STATE new_state);
static void conn_close(conn *c);
void event_handler(int fd, short which, void *arg);
static int set_socket_nonblock(int fd);
static int set_socket_reusable(int fd);

int main(int argc, char **argv)
{
    main_base = event_init();
    
    int sockfd = socket_setup(NPORT);
    if (sockfd < 0){
        perror("Failed to create socket.");
        return -1;
    }
    
    conn *c = conn_new(sockfd,CONN_STATE_LISTENING,main_base);
    if (c == NULL){
        fprintf(stderr,"Failed to create socket accept.\n");
        return -1;
    }
    
    
    thread_init(MAXTHREADS,main_base);
    
    /*wait for threads to setup completely.*/
    usleep(100000);
    
    event_base_loop(main_base,0);
    
    return 0;
}

static int set_socket_nonblock(int fd)
{
    int flag;
    flag = fcntl(fd,F_GETFL,NULL);
    if (flag < 0){
        return -1;
    }
    
    flag |= O_NONBLOCK;
    
    if (fcntl(fd,F_SETFL,flag) < 0){
        return -1;
    }
    
    return 0;
}

static int set_socket_reusable(int fd)
{
    int reuse_on = 1;
    return setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&reuse_on,sizeof(reuse_on));
}

int socket_setup(int nPort)
{
    int listenfd;
    struct sockaddr_in listen_addr;
    int reuse = 1;
    
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd < 0){
        fprintf(stderr,"Failed to create socket.\n");
        return -1;
    }
    
    /*set socket reuseable*/
    if (set_socket_reusable(listenfd) < 0){
        fprintf(stderr,"Failed to set listening socket re-usable.\n");
        return -1;
    }
    
    /*set socket non-blocking*/
    if (set_socket_nonblock(listenfd) < 0){
        fprintf(stderr,"Failed to set listening socket non-blocking.\n");
        return -1;
    }
    
    memset(&listen_addr,0,sizeof(struct sockaddr_in));
    listen_addr.sin_family      = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port        = htons(nPort);
    if (bind(listenfd,(struct sockaddr*)&listen_addr,sizeof(listen_addr)) < 0){
        fprintf(stderr,"Failed to bind the listening socket fd.\n");
        return -1;
    }
    
    if (listen(listenfd,BACKLOG) < 0){
        fprintf(stderr,"Failed to set the listening socket to listen.\n");
        return -1;
    }
    
    return listenfd;
}

static last_thread = -1;

void dispatch_conn(int sfd,CONN_STATE state){
    char buf[1];
    CQ_ITEM *item = malloc(sizeof(CQ_ITEM));
    item->fd = sfd;
    item->state = state;
    
    int tid = (last_thread + 1)%MAXTHREADS;
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
                
                dispatch_conn(connfd,CONN_STATE_READ);
                
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
                dispatch_conn(c->fd,CONN_STATE_WRITE);
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
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connfd;
    char buffer[1024];
    int n = 0;
    char sendmsg[] = "Hi,this is server, can you receive me?";

#if 0    
//    printf("event_handler fd = %d\n",fd);
    switch (c->state){
        case CONN_STATE_LISTENING:
            connfd = accept(c->fd,(struct sockaddr*)&client_addr,&client_len);
            /*check connfd*/
            if (connfd == -1){
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    
                }
                else if (errno == EMFILE){/*exceed per-process limit of file descriptions.*/
                    fprintf(stderr,"Too many opened connections.\n");
                }
                else{
                    fprintf(stderr,"accept error.\n");
                }
            }
            
            if (set_socket_nonblock(connfd) < 0){
                close(connfd);
            }
            else{
                printf("connfd = %d\n",connfd);
                dispatch_conn(connfd,CONN_STATE_READ);
            }
            break;
        case CONN_STATE_READ:
            
            n = read(c->fd,buffer,1024);
            if (n == -1){
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    /*do nothing*/
                }
                else{
                    /*read error,connection dropped.*/
                    
                }
            }
            else if (n == 0){
                
            }
            else{
                printf("c->fd = %d received data:%s with thread_id=0x%x\n",c->fd,buffer,pthread_self());
                memset(buffer,0,1024);
            }
            
            dispatch_conn(c->fd,CONN_STATE_WRITE);
            break;
        case CONN_STATE_WRITE:
            printf("c->fd=%d server sent: %s\n",c->fd,sendmsg);
            write(c->fd,sendmsg,strlen(sendmsg));
            
            break;
        case CONN_STATE_CLOSE:
            conn_close(c);
            break;
        default:
            break;
    }
#else
    event_FSM(c);
#endif
    return;
}


static conn *conn_new(int fd,CONN_STATE init_state,struct event_base *base)
{
    conn *c = malloc(sizeof(conn));

    c->fd = fd;
    c->base = base;
    c->state = init_state;
    

    if (c->state == CONN_STATE_WRITE){
        /*don't set the flag as EV_WRITE|EV_PERSIST*/
        event_set(&c->event,fd,EV_WRITE,event_handler,(void*)c);
    }
    else{
        event_set(&c->event,fd,EV_READ|EV_PERSIST,event_handler,(void*)c);
    }
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
    char buffer[1024];
    CQ_ITEM *item;
    
    char buf[1];
    if (read(fd,buf,1) != 1){
        perror("can't read from libevent pipe.");
    }
    
//    printf("hi, I get it from 0x%x.\n",this->thread_id);
    
    item = cq_pop(&this->new_conn_queue);
    
    if (item != NULL){
        conn *c = conn_new(item->fd,item->state,this->base);
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


int thread_init(int nthreads,struct event_base *main_base)
{
    int i;
    
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



