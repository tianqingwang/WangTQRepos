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

#define  DEBUG    0
#define  MAX_NOFILE  4096

struct client{
    int fd;
//    struct bufferevent *buf_ev;
    struct event ev_read;
    struct event ev_write;
};

int SetNonBlock(int fd);
void on_accept(evutil_socket_t fd, short what, void *arg);
#if 0
void on_read(evutil_socket_t fd, short what, void *arg);
#else
void on_read(struct bufferevent *bev, void *arg);
#endif
void on_write(evutil_socket_t fd, short what, void *arg);

//struct event_base *base;

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
    
    struct event_base *base = event_base_new();
    
    listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if (listen_fd < 0){
        return -1;
    }
#if 0    
    int reuseaddr_on = 1;
    if (setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr_on,sizeof(reuseaddr_on)) == -1){
        printf("Error: set socket option failed\n");
    }
#endif
    
    evutil_make_listen_socket_reuseable(listen_fd);
    
    struct sockaddr_in listen_addr;
    memset( &listen_addr,0,sizeof( listen_addr ) );
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons( 5000 );
    if( bind( listen_fd,(struct sockaddr*)&listen_addr,sizeof( listen_addr ) ) < 0)
    {
        printf("Error: bind failed\n");
    }
    if( listen( listen_fd,500 ) < 0 )
    {
        printf("Error: listen failed\n");
        return -1;
    }
#if 0
    if( SetNonBlock(listen_fd) < 0 )
    {
        printf("Error: SetNonBlock failed\n");
        return -1;
    }
#endif
    evutil_make_socket_nonblocking(listen_fd);
    
#if DEBUG    
    printf("libevent backend method %s\n",event_base_get_method(base));
    
    enum event_method_feature f;
    
    f = event_base_get_features(base);
    if ((f & EV_FEATURE_ET)){
        printf("ET mode\n");
    }
#endif
    
    ev_accept = event_new(base,listen_fd,EV_READ|EV_PERSIST,on_accept,(void*)base);
    
    
    event_add(ev_accept,NULL);
    
    event_base_dispatch(base);
    
}


int SetNonBlock(int fd)
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

void on_accept(evutil_socket_t fd, short what, void *arg){
    int connfd;
    struct sockaddr_in client_addr;
    struct client *client;
    
    struct event_base *base = (struct event_base*)arg;
    
    socklen_t client_len = sizeof(client_addr);
    
    connfd = accept(fd, (struct sockaddr*)&client_addr,&client_len);
    if (connfd == -1){
        printf("Error: accept failed\n");
    }
    
    if (SetNonBlock(connfd) < 0){
        printf("Error: set client non-blocking failed\n");
    }
    
    printf("connfd=%d\n",connfd);
    
    client = malloc(sizeof(struct client));
    if (!client){
        printf("malloc failed\n");
    }
    
    client->fd = connfd;
    
#if 0
    event_set(&client->ev_read,connfd,EV_READ|EV_PERSIST,on_read,client);
    event_base_set(base,&client->ev_read);
    event_add(&client->ev_read,NULL);
#else
    struct bufferevent *bev = bufferevent_socket_new(base,connfd,BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev,on_read,NULL,NULL,arg);
    bufferevent_enable(bev,EV_READ|EV_WRITE|EV_PERSIST);
#endif
}

#if 0
void on_read(evutil_socket_t fd, short what, void *arg)
{ 
    struct client *pClient = (struct client*)arg;
    char buf[4096] = {0};
    
    int len = read(fd,buf,4096);
    if (len == 0){
        close(fd);
        event_del(&pClient->ev_read);
        free(pClient);
        return;
    }
    else if (len < 0){
        close(fd);
        event_del(&pClient->ev_read);
        free(pClient);
        return;
    }
    else{
        printf("You said %s\n",buf);
    }
    
}
#else
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
#endif

void on_write(evutil_socket_t fd, short what, void *arg)
{
    
}


