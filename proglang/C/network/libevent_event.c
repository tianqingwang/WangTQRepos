#include <unistd.h>
#include <stdio.h>
#include <event.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include "libevent_event.h"
#include "workqueue.h"
#include "log.h"

#define THREAD_NUM    (8)

struct event_base *main_base;
struct event       main_event;
struct event       clock_event;

/*store the received data from client.*/
static user_data_t *pUserData = NULL;

struct timeval tv_read_timeout={5,0};

void on_accept(int fd, short which, void *args);
void on_read(struct bufferevent *bev, void *args);
void on_error(struct bufferevent *bev, short which, void *args);
void check_timeout(int fd, short which, void *args);

static void *write_process(void *arg);

void main_loop(int sockfd)
{
    struct timeval tv={20,0};
    enum event_method_feature f;
    
    workqueue_init(THREAD_NUM);
    
    main_base = event_init();
    
    /*some information of libevent.*/
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

void main_loop_exit()
{
    event_base_loopexit(main_base,NULL);
    
    event_del(&main_event);
    event_del(&clock_event);
    
    event_base_free(main_base);
    
    if (pUserData != NULL){
        free(pUserData);
    }
}

void on_accept(int fd, short which, void *args)
{
    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    connfd = accept(fd,(struct sockaddr*)&client_addr,&client_len);
    if (connfd == -1){
        if (errno == EAGAIN || errno == EINTR){
            logInfo(LOG_WARN,"accept error: EAGAIN or EINTR");
        }
        else{
            logInfo(LOG_ERR,"accept error: fd = %d, %s",connfd,strerror(errno));
        }
    }
    
    logInfo(LOG_INFO,"accept fd=%d",connfd);
    
    /*libevent needs nonblocking socket fd.*/
    if (set_socket_nonblock(connfd) == -1){
        logInfo(LOG_ERR,"failed to set NONBLOCK.");
        close(connfd);
    }
    
    fd_insert(connfd);
    
    struct bufferevent *bev = bufferevent_socket_new(main_base,connfd,BEV_OPT_CLOSE_ON_FREE);
    if (bev == NULL){
        logInfo(LOG_ERR,"bufferevent_socket_new error.");
    }
    
    bufferevent_setcb(bev,on_read,NULL,on_error,NULL);
    
    bufferevent_enable(bev,EV_READ|EV_PERSIST);
}

void on_read(struct bufferevent *bev, void *args)
{
    int i;
    /*get connected socket fd.*/
    int fd = bufferevent_getfd(bev); 
    /*get input buffer*/
    struct evbuffer *input = bufferevent_get_input(bev);
    
    size_t len = evbuffer_get_length(input);
    if (len){
        /*todo: fix data reading operation by your requirement.*/
        if (fd < 0){
            return ;
        }
        
        fd_update_last_time(fd,time(NULL));
        
        i=fd;
        
        pUserData[i].fd = fd;
        
        if (pUserData[i].pdata != NULL){
            free(pUserData[i].pdata);
        }
        pUserData[i].pdata = (char*)malloc(sizeof(char)*(len+1));
        if (pUserData[i].pdata == NULL){
            logInfo(LOG_ERR,"failed to allocate memory for pdata of pUserData.");
            return;
        }
        
        evbuffer_remove(input,pUserData[i].pdata,len);
        
        pUserData[i].pdata[len] = '\0';
        pUserData[i].datalen    = len;
        
        fprintf(stdout,"\nfd=%d read string:%s\n",fd,pUserData[i].pdata);
        
        workqueue_add_job(write_process,(void*)&pUserData[i]);
    }
    
}

void on_error(struct bufferevent *bev, short which, void *args)
{
    if (which & (BEV_EVENT_EOF)){
        logInfo(LOG_ERR,"connection closed,fd=%d,error:%s",bufferevent_getfd(bev),evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        
    }
    else if (which & (BEV_EVENT_ERROR)){
        logInfo(LOG_ERR,"BEV_EVENT_ERROR,fd=%d,error:%s",bufferevent_getfd(bev),evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        //bufferevent_free(bev);
    }
    
    bufferevent_free(bev);

}

void check_timeout(int fd, short which, void *args)
{
}

void set_max_connection(int nMaxConnection)
{
    if (pUserData != NULL){
        free(pUserData);
    }
    
    pUserData = (user_data_t*)malloc(sizeof(user_data_t)*nMaxConnection);
    if (pUserData == NULL){
        logInfo(LOG_ERR,"failed to allocate memory.");
        exit(1);
    }
    
    int i=0; 
    for (i=0; i<nMaxConnection; i++){
        pUserData[i].fd = -1;
        pUserData[i].datalen = 0;
    }
}

static void signal_handler(int sig)
{
    if (sig == SIGINT){
        workqueue_shutdown();
        main_loop_exit();
        fd_free();
        endLogInfo();
    }
}

void signal_process()
{
    struct sigaction action={
        .sa_handler = signal_handler,
        .sa_flags   = 0,
    };
    
    sigemptyset(&action.sa_mask);
    
    sigaction(SIGINT,&action,NULL);
}

/*This is the callback function for thread pool. Please fill function
 *by your requirement. Also, you can define many callback functions
 *for different arguments as you need.
 */
static void *write_process(void *arg)
{
    
    user_data_t *user_data = (user_data_t*)arg;
    
    printf("write_process:%s with fd=%d,thread=0x%x\n",user_data->pdata,user_data->fd,pthread_self());
    
    char echobuf[1024] = {0};
    sprintf(echobuf,"echo: %s with fd=%d.",user_data->pdata,user_data->fd);
    write(user_data->fd,echobuf,strlen(echobuf));
    memset(echobuf,0,1024);
    //usleep(10000);
    usleep(100000);

}
