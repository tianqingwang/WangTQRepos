#include <unistd.h>
#include <stdio.h>
#include <event.h>
#include <errno.h>
#include <stdlib.h>
#include "libevent_event.h"
#include "workqueue.h"
#include "log.h"

#define THREAD_NUM    (4)

struct event_base *main_base;
struct event       main_event;
struct event       clock_event;

static workqueue_t workqueue;


struct timeval tv_read_timeout={5,0};

void on_accept(int fd, short which, void *args);
void on_read(struct bufferevent *bev, void *args);
void on_error(struct bufferevent *bev, short which, void *args);
void check_timeout(int fd, short which, void *args);
//static void write_process(job_t *job);
static void *write_process(void *arg);
void main_loop(int sockfd)
{
    struct timeval tv={20,0};
    enum event_method_feature f;

    fd_init();
    
//    workqueue_init(&workqueue,THREAD_NUM);
    workqueue_init(THREAD_NUM);
    
    main_base = event_init();
    
    /*list method and mode.*/
    fprintf(stdout,"Using Libevent with backend method:%s.\n",event_base_get_method(main_base));
    
    f=event_base_get_features(main_base);
    if (f & EV_FEATURE_ET){
        fprintf(stdout,"Libevent uses Edge-triggered mode.\n");
    }
    if (f & EV_FEATURE_O1){
        fprintf(stdout,"Libevent uses O(1) event notification.\n");
    }
    if (f & EV_FEATURE_FDS){
        fprintf(stdout,"Libevent supports all FD types.\n");
    }
    
    /*set main_event to main_base for accepting new connection.*/
    event_set(&main_event, sockfd,EV_READ|EV_PERSIST, on_accept,NULL);
    event_base_set(main_base,&main_event);
    event_add(&main_event,0);
    
    /*set clock_event to main_base for checking timeout socket.*/
    event_set(&clock_event,-1,EV_TIMEOUT|EV_PERSIST,check_timeout,NULL);
    event_base_set(main_base,&clock_event);
    event_add(&clock_event,&tv);
    
    event_base_loop(main_base,0);
}

void on_accept(int fd, short which, void *args)
{
    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    connfd = accept(fd,(struct sockaddr*)&client_addr,&client_len);
    if (connfd == -1){
        if (errno == EAGAIN){
            logInfo(LOG_WARN,"accept errno is EAGAIN");
        }
        else if (errno == EMFILE){
            logInfo(LOG_ERR,"Too many open connections.");
        }
        else{
            logInfo(LOG_ERR,"accept error.");
        }
    }
    
    /*libevent needs nonblocking socket fd.*/
    if (set_socket_nonblock(connfd) == -1){
        logInfo(LOG_ERR,"failed to set NONBLOCK.");
        close(connfd);
    }
    
    printf("accept fd=%d\n",connfd);
    
    fd_insert(connfd);
    
    struct bufferevent *bev = bufferevent_socket_new(main_base,connfd,BEV_OPT_CLOSE_ON_FREE);
    if (bev == NULL){
        logInfo(LOG_ERR,"bufferevent_socket_new error.");
    }
    
    bufferevent_setcb(bev,on_read,NULL,on_error,NULL);
    bufferevent_set_timeouts(bev,&tv_read_timeout,NULL);
    bufferevent_enable(bev,EV_READ|EV_PERSIST);
}

void on_read(struct bufferevent *bev, void *args)
{
    
    /*get connected socket fd.*/
    int fd = bufferevent_getfd(bev); 
    /*get input buffer*/
    struct evbuffer *input = bufferevent_get_input(bev);
    
    size_t len = evbuffer_get_length(input);
    if (len){
        /*get all data*/
        user_data_t *user_data = (user_data_t*)malloc(sizeof(user_data_t));
//        job_t       *job = (job_t*)malloc(sizeof(job_t));
        
        user_data->pdata = (char*)malloc(sizeof(char)*(len+1));
        
        if (user_data->pdata == NULL){
            logInfo(LOG_ERR,"failed to allocate memory for user_data.");
        }
        evbuffer_remove(input,user_data->pdata,len);
        user_data->pdata[len] = '\0';
        user_data->datalen = len;
        user_data->fd      = fd;
        
//        job->job_function = write_process;
//        job->arg          =(user_data);
        printf("add fd=%d value to workqueue.\n",fd);
       // workqueue_add_job(&workqueue,&job);
 //      workqueue_add_job(&job);
 //       workqueue_add_job(job);
        workqueue_add_job(write_process,(void*)user_data);

    }
    
}

void on_error(struct bufferevent *bev, short which, void *args)
{
    if (which & (BEV_EVENT_ERROR)){
        logInfo(LOG_ERR,"bufferevent error.");
        bufferevent_free(bev);
    }
}

void check_timeout(int fd, short which, void *args)
{
}

static void *write_process(void *arg)
{
    user_data_t *user_data = (user_data_t*)arg;
    printf("user_data=%p\n",user_data);
    printf("write_process:%s with fd=%d,thread=0x%x\n",user_data->pdata,user_data->fd,pthread_self());
    
    int i;
    for (i=0; i<600000; i++){
    }
}
