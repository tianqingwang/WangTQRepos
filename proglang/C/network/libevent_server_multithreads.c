#include <unistd.h>
#include <fcntl.h>
#include <event.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include "workqueue.h"

#define  DEBUG    0
#define  MAX_NOFILE  4096

#define  BACKLOG     (5)
#define  PORT_NUM    (5000)
#define  WORKQUEUE_SIZE (2)


typedef struct client{
    int fd;
    struct event_base *evbase;
    struct bufferevent *buf_ev;
#if 1
    struct event ev_read;
    struct event ev_write;
#endif
    int receive_fd;
    int send_fd;
}client_t;

struct LIBEVENT_THREAD{
    int receive_fd; /*pipe end fd for receive*/
    int send_fd;    /*pipe end fd for send*/
    struct event_base *evbase;
    struct event ev_read;
    struct event ev_write;
};

static struct LIBEVENT_THREAD threads;

static workqueue_t workqueue;

void on_accept(evutil_socket_t fd, short what, void *arg);
void sigterm_func(evutil_socket_t fd, short what, void *arg);
void on_read(struct bufferevent *bev, void *arg);
#if 0
void on_write(evutil_socket_t fd, short what, void *arg);
#endif

void event_error(struct bufferevent *bev, short events, void *arg);

static void job_server_func(job_t *job);
void closeAndFreeClient(client_t *client);
static void closeClient(client_t *client);

int main(int argc, char *argv[])
{
    int listen_fd;
    struct event *ev_accept;
    struct rlimit rt;
    
    
    if (getrlimit(RLIMIT_NOFILE,&rt) == -1){
        printf("getrlimit\n");
    }
    
    printf("rlim_max=%d,rlim_cur=%d\n",rt.rlim_max,rt.rlim_cur);
    
    rt.rlim_max = rt.rlim_cur = MAX_NOFILE;
    
    if (setrlimit(RLIMIT_NOFILE,&rt) == -1){
        printf("setrlimit\n");
    }
    
    /*setup socket*/
    listen_fd  = socket_setup(PORT_NUM);
    if (listen_fd < 0){
        perror("failed to create socket.");
        return -1;
    }
    
    /*init workqueue*/
    if (workqueue_init(&workqueue,WORKQUEUE_SIZE) < 0){
        perror("failed to create work queue.");
        close(listen_fd);
        workqueue_shutdown(&workqueue);
        return -1;
    }
    
    struct event_base *base = event_base_new();
    if (base == NULL){
        perror("failed to create base event.");
        close(listen_fd);
        workqueue_shutdown(&workqueue);
        return -1;
    }
    
    ev_accept = event_new(base,listen_fd,EV_READ|EV_PERSIST,on_accept,(void*)&workqueue);
    
    event_add(ev_accept,NULL);
    
    printf("Server running.\n");
    /*start event loop, program hang up here.*/
    event_base_dispatch(base);
    
    
}

void on_accept(evutil_socket_t fd, short what, void *arg){
    int connfd;
    struct sockaddr_in client_addr;
    struct client *client;
    job_t  *job;

    /*for multi-threads version, the argument is workqueue.*/
    workqueue_t *pworkqueue = (workqueue_t*)arg;
    
    socklen_t client_len = sizeof(client_addr);
    
    connfd = accept(fd, (struct sockaddr*)&client_addr,&client_len);
    if (connfd == -1){
        perror("accept failed.");;
        return ;
    }
    
    if (evutil_make_socket_nonblocking(connfd) < 0){
        perror("failed to set client socket to non-blocking.");
        close(connfd);
        return;
    }
    
    printf("connfd=%d\n",connfd);
  
    client = malloc(sizeof(struct client));
    if (!client){
        perror("failed to allocate memory for client");
        close(connfd);
        return;
    }
    
    memset(client,0,sizeof(struct client));
    
    client->evbase = event_base_new();
    if (client->evbase == NULL){
        perror("failed to create client socket event base.");
        close(connfd);
        return ;
    }
#if 1  
    /*set client*/
    client->fd = connfd;
    client->buf_ev = bufferevent_socket_new(client->evbase,connfd,BEV_OPT_CLOSE_ON_FREE);
    
    if (client->buf_ev == NULL){
        perror("failed to allocate buffer event.");
        close(connfd);
        return;
    }
    
//    bufferevent_setcb(client->buf_ev,on_read,NULL,event_error,arg);
    bufferevent_setcb(client->buf_ev,on_read,NULL,event_error,client);
    bufferevent_enable(client->buf_ev,EV_READ);
    
    job = (job_t*)malloc(sizeof(job_t));
    if (job == NULL){
        perror("failed to allocate memory for job_t");
        return ;
    }
    
    job->job_function = job_server_func;
    job->user_data    = client;
    
    workqueue_add_job(pworkqueue,job);
#else
    client->fd = connfd;
    event_set(&client->ev_read,client->fd,EV_READ,on_read,arg);
    event_base_set(client->evbase,&client->ev_read);
    event_add(&client->ev_read);
#endif
}

void signal_handler(int signum)
{
    
}

void job_server_func(job_t *job)
{
    client_t *client = (client_t*)job->user_data;
    printf("client->fd = %d\n",client->fd);
    event_base_dispatch(client->evbase);

    closeAndFreeClient(client);
    free(job);
}

static void closeClient(client_t *client)
{
    if (client != NULL){
        if (client->fd > 0)
        {
            close(client->fd);
            client->fd = -1;
        }
    }
}

void closeAndFreeClient(client_t *client){
    if (client == NULL) return;
    
    closeClient(client);
    
    if (client->buf_ev != NULL){
        bufferevent_free(client->buf_ev);
        client->buf_ev = NULL;
    }
    
    if (client->evbase != NULL){
        event_base_free(client->evbase);
        client->evbase = NULL;
    }
    
    free(client);
}


void on_read(struct bufferevent *bev, void *arg)
{
    client_t *client = arg;
    int n;
    char line[1024]={0};
//    evutil_socket_t fd = bufferevent_getfd(bev);
    int fd = client->fd;
    
    while((n=bufferevent_read(bev,line,1024)) > 0){
        line[n] = '\0';
        printf("fd=%d,read line:%s\n",fd,line);
        bufferevent_write(bev,line,n);
        memset(line,0,1024);
    }
}

#if 0
void on_write(evutil_socket_t fd, short what, void *arg)
{
    
}
#endif

void event_error(struct bufferevent *bev, short events, void *arg)
{
    client_t *pClient = (client_t*)arg;
    
    closeClient(pClient);
}

int socket_setup(int nPort)
{
    int listenfd;
    struct sockaddr_in listen_addr;
    int reuse = 1;
    
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd < 0){
        return -1;
    }
    
    /*set socket reuseable*/
    evutil_make_listen_socket_reuseable(listenfd);
    /*set socket non-blocking*/
    evutil_make_socket_nonblocking(listenfd);
    
    memset(&listen_addr,0,sizeof(struct sockaddr_in));
    listen_addr.sin_family      = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port        = htons(nPort);
    if (bind(listenfd,(struct sockaddr*)&listen_addr,sizeof(listen_addr)) < 0){
        return -1;
    }
    
    if (listen(listenfd,BACKLOG) < 0){
        return -1;
    }
    
    return listenfd;
}

/***********************以下代码为试验代码*****************/
#if 1
static void create_worker(void *(*func)(void*),void *arg)
{
    pthread_t thread;
    pthread_attr_t attr;
    int ret ;
    
    pthread_attr_init(&attr);
    
    ret = pthread_create(&thread,&attr,func,arg);
    if (ret != 0){
        perror("Failed to create thread.");
        exit(1);
    }
    
}

static void thread_libevent_process(int fd, short which,void *arg)
{
    LIBEVENT_THREAD *this = arg;
    
    char buf;
    if (read(fd,buf,1) != 1){
        perror("can't read from libevent pipe.");
    }
    
    
}

static void *worker_libevent(void *arg)
{
    LIBEVENT_THREAD *me = arg;
    
    event_base_loop(me->evbase,0);
    return NULL;
}

int thread_init(int nthreads,struct event_base *main_base)
{
    int i;
    
    threads = malloc(sizeof(LIBEVENT_THREAD)*nthreads);
    if (threads == NULL){
        return -1;
    }
    
    for (i=0; i<nthreads; i++){
        int fds[2];
        if (pipe(fds)){
            perror("Failed to create pipe.");
            return -1;
        }
        threads[i].receive_fd = fds[0];
        threads[i].send_fd    = fds[1];
        
        setup_thread(&threads[i]);
    }
    
    for (i=0; i<nthreads; i++){
        create_worker(worker_libevent,&threads[i]);
    }
}

int setup_thread(LIBEVENT_THREAD *thread)
{
    thread->evbase = event_init();
    if (thread->evbase == NULL){
        perror("Failed to allocate event base.");
        return -1;
    }
    
    event_set(&thread->ev_read,thread->receive_fd,EV_READ|EV_PERSIST,thread_libevent_process,thread);
    event_base_set(thread->evbase,&thread->ev_read);
    if (event_add(&thread->ev_read,0) == -1){
        perror("Failed to add event.");
        return -1;
    }
    
    
}

/*********************试验代码结束*********************/
#endif

