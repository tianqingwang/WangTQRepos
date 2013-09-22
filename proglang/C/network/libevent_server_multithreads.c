#include <unistd.h>
#include <fcntl.h>
#include <event.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include "workqueue.h"

#define  DEBUG    0
#define  MAX_NOFILE  4096

#define  BACKLOG     (5)
#define  PORT_NUM    (5000)
#define  MAX_THREADS (8)
#define  WORKQUEUE_SIZE (8)


typedef struct client{
    int fd;
    struct event_base *evbase;
    struct bufferevent *buf_ev;
    struct event ev_read;
    struct event ev_write;
}client_t;

static workqueue_t workqueue;

void on_accept(evutil_socket_t fd, short what, void *arg);

void on_read(struct bufferevent *bev, void *arg);

void on_write(evutil_socket_t fd, short what, void *arg);

static void job_server_func(job_t *job);
void closeAndFreeClient(client_t *client);

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

#if 0    
    ev_accept = event_new(base,listen_fd,EV_READ|EV_PERSIST,on_accept,(void*)base);
#endif
    
    ev_accept = event_new(base,listen_fd,EV_READ|EV_PERSIST,on_accept,(void*)&workqueue);
    
    event_add(ev_accept,NULL);
    
    printf("Server begins running.\n");
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
    
    /*set client*/
    client->fd = connfd;
    client->buf_ev = bufferevent_socket_new(client->evbase,connfd,BEV_OPT_CLOSE_ON_FREE);
    
    if (client->buf_ev == NULL){
        perror("failed to allocate buffer event.");
        close(connfd);
        return;
    }
    
    bufferevent_setcb(client->buf_ev,on_read,NULL,NULL,arg);
    bufferevent_enable(client->buf_ev,EV_READ);
    
    job = (job_t*)malloc(sizeof(job_t));
    if (job == NULL){
        perror("failed to allocate memory for job_t");
        return ;
    }
    
    job->job_function = job_server_func;
    job->user_data    = client;
    
    workqueue_add_job(pworkqueue,job);
}

void job_server_func(job_t *job)
{
    client_t *client = (client_t*)job->user_data;
    
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
    int n;
    char line[1024]={0};
    evutil_socket_t fd = bufferevent_getfd(bev);
    
    while((n=bufferevent_read(bev,line,1024)) > 0){
        line[n] = '\0';
        printf("fd=%d,read line:%s\n",fd,line);
        bufferevent_write(bev,line,n);
    }
}


void on_write(evutil_socket_t fd, short what, void *arg)
{
    
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


