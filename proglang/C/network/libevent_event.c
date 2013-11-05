#include <unistd.h>
#include <stdio.h>
#include <event.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libevent_event.h"
#include "workqueue.h"
#include "log.h"

#define THREAD_NUM    (8)

struct event_base *main_base;
struct event       main_event;

/*store the received data from client.*/
static user_data_t *pUserData = NULL;
struct timeval tv_read_timeout={5,0};

void on_accept(int fd, short which, void *args);
void on_read(struct bufferevent *bev, void *args);
void on_error(struct bufferevent *bev, short which, void *args);
void check_timeout(int fd, short which, void *args);
void get_packet(int fd);

static void *write_process(void *arg);

void main_loop(int sockfd)
{
    struct timeval tv={20,0};
    enum event_method_feature f;
    
    workqueue_init(THREAD_NUM);
    
    main_base = event_init();

#if 0   
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
#endif
    
    /*set main_event to main_base for accepting new connection.*/
    event_set(&main_event, sockfd,EV_READ|EV_PERSIST, on_accept,NULL);
    event_base_set(main_base,&main_event);
    event_add(&main_event,0);
   
    event_base_loop(main_base,0);
}

void main_loop_exit()
{
    event_base_loopexit(main_base,NULL);
    
    event_del(&main_event);   
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
    
    
    /*default max single read size is 4096*/
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
    int ncopied;
    int packlen = 0;
    
    char *pStr = NULL;
    /*get connected socket fd.*/
    int fd = bufferevent_getfd(bev); 
    
    struct evbuffer *input = bufferevent_get_input(bev);

    size_t len = evbuffer_get_length(input);
   

    if (len > 0 && fd > 0){
        i = fd;

        pUserData[i].fd = fd;
        
        ncopied = evbuffer_remove(input,pUserData[i].data + pUserData[i].nstart, RBUF_SIZE - pUserData[i].nstart);       
       
        if (ncopied < 0){
            /*todo:failed,need data error process.*/
            evbuffer_free(input);
            return ;
        }
        else{
            pUserData[i].nstart += ncopied;
            /*Get packet here, you can modify by your request.*/
            get_packet(fd);
        }
       
    }
}

/*You should modify this function according to your protocol*/
void get_packet(int fd)
{
    int   i,packlen;
    char *pStr = NULL;
    
    i = fd;
    
    while(pUserData[i].nstart > 0 && ((pStr = memchr(pUserData[i].data,'\n',pUserData[i].nstart)) != NULL)){
        user_data_t *userdata = malloc(sizeof(user_data_t));
        
        memset(userdata,0,sizeof(user_data_t));
        
        packlen = userdata->datalen = pStr - pUserData[i].data + 1;
        userdata->fd = fd;
        memcpy(userdata->data,pUserData[i].data,userdata->datalen);
        userdata->data[userdata->datalen] = '\0';
        
        //fprintf(stdout,"%s",userdata->data);
        //fflush(stdout);
        workqueue_add_job(write_process,userdata);
        
        if (pUserData[i].nstart - packlen > 0 && (pStr + 1) != NULL){
            memmove(pUserData[i].data,pStr+1,pUserData[i].nstart - packlen);
        }
        pUserData[i].nstart -= packlen;
    }
    /*there doesn't exist boundary, discard data.*/
    if (pUserData[i].nstart == RBUF_SIZE){
        pUserData[i].fd      = -1;
        pUserData[i].nstart  = 0;
        pUserData[i].datalen = 0;
        memset(pUserData[i].data,0,sizeof(pUserData[i].data));
    }
}

void on_error(struct bufferevent *bev, short which, void *args)
{
    int fd = bufferevent_getfd(bev); 
    
    if (which & (BEV_EVENT_EOF)){
        logInfo(LOG_ERR,"connection closed,fd=%d,error:%s",bufferevent_getfd(bev),evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        
    }
    else if (which & (BEV_EVENT_ERROR)){
        logInfo(LOG_ERR,"BEV_EVENT_ERROR,fd=%d,error:%s",bufferevent_getfd(bev),evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
        //bufferevent_free(bev);
    }
    
    bufferevent_free(bev);
    
    pUserData[fd].fd = -1;
    pUserData[fd].datalen = 0;
    pUserData[fd].nstart = 0;
    memset(pUserData[fd].data,0,sizeof(pUserData[fd].data));
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
        pUserData[i].fd      = -1;
        pUserData[i].datalen = 0;
        pUserData[i].nstart  = 0;
        memset(pUserData[i].data,0,sizeof(pUserData[i].data));
    }
}

static void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM){
        workqueue_shutdown();
        main_loop_exit();
        fd_free();
        endLogInfo(); 
    }
    else if (sig == SIGPIPE){
        /*This is important to avoid system down.*/
        fprintf(stderr,"pipe broken.\n");
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
    sigaction(SIGTERM,&action,NULL);
    /*catch SIGPIPE to avoid system down when read/write pipe is broken.*/
    sigaction(SIGPIPE,&action,NULL);
}

/*This is the callback function for thread pool. Please fill function
 *by your requirement. Also, you can define many callback functions
 *for different arguments as you need.
 */
static void *write_process(void *arg)
{
    int res = -1;
    user_data_t *user_data = (user_data_t*)arg;
    //fprintf(stdout,"%s",user_data->data);
    //fflush(stdout);
    write(user_data->fd,user_data->data,user_data->datalen);
    
}
