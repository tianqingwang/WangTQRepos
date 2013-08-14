#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>

#define MAXEVENTS    (2000)
#define BUFSIZE      (1024*1024)

static struct epoll_event *event_list;
typedef struct {
    int  sockfd;
    char buffer[BUFSIZE];
    int  offset;
    int  len;
}EVENT_S;

void setnonblocking(int sockfd)
{
    int flag;
    flag = fcntl(sockfd,F_GETFL);
    if (flag < 0){
        printf("fcntl(F_GETFL) fail.\n");
        exit(-1);
    }
    
    flag |= O_NONBLOCK;
    if (fcntl(sockfd,F_SETFL,flag) < 0){
        printf("fcntl(F_SETFL) failed.\n");
        exit(-1);
    }
}


int main(int argc, char* argv[]){
    struct epoll_event event;
    int mysockfd = 0;
    int nevents = 0;
    int n = 0;
    
    EVENT_S *rwEvent = NULL;
    
    int listenfd = setup_socket(NULL,58888,AF_INET,SOCK_STREAM,0);
    if (listenfd < 0){
        return -1;
    }
    
    int epfd = epoll_create(MAXEVENTS);
    if (epfd < 0){
        perror("epoll create");
        return -1;
    }
    
    event_list = (struct epoll_event*)malloc(MAXEVENTS*sizeof(struct epoll_event));
    
    if (event_list == NULL){
        perror("event_list error");
        return -1;
    }
    
    EVENT_S *event_s = malloc(sizeof(EVENT_S));
    memset(event_s,0,sizeof(EVENT_S));
    event_s->sockfd = listenfd;
    
    event.data.ptr = event_s;
    event.events = EPOLLIN|EPOLLET; /*read and edge trigger*/
    
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD,listenfd,&event);
    if (ret == -1){
        perror("epoll_ctl error");
        return -1;
    }
    
    while(1){
        nevents = epoll_wait(epfd, event_list,MAXEVENTS,500);
        printf("epoll_wait events=%d\n",nevents);
        for (n=0; n<nevents; n++){
            if (event_list[n].events & (EPOLLERR | EPOLLHUP)){
                fprintf(stderr,"epoll wait error in event_list[%d]\n",n);
                close(event_list[n].data.fd);
                continue;
            }
            else{
                rwEvent = (EVENT_S*)event_list[n].data.ptr;
                if (rwEvent->sockfd == listenfd){
                    /*accept*/
                    printf("accept\n\n");
                    struct sockaddr_in conn_addr;
                    int len = sizeof(conn_addr);
                    int connfd = accept(listenfd,(struct sockaddr*)&conn_addr,(socklen_t*)&len);
                    /*set connfd as noblock*/
                    setnonblocking(connfd);
                    
                    //printf("connect  port = %d\n",ntohs(conn_addr.sin_port));
                    
                    if (connfd > 0){
                        EVENT_S *event_conn = malloc(sizeof(EVENT_S));
                        memset(event_conn,0,sizeof(EVENT_S));
                        
                        event_conn->sockfd = connfd;
                        
                        event.data.ptr = event_conn;
                        event.events = EPOLLIN | EPOLLET;
                        epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&event);
                    }
                }
                else if (event_list[n].events & EPOLLIN){
                    /*read event*/
                    printf("read\n\n");
                    //mysockfd = event_list[n].data.fd;
                    rwEvent = (EVENT_S*)event_list[n].data.ptr;
                    
                    mysockfd = rwEvent->sockfd;
                    if (mysockfd < 0){
                        continue;
                    }
                    
                    int recvlen = recv(mysockfd,rwEvent->buffer,BUFSIZE,0);
                    if (recvlen == -1){
                        if (errno != EAGAIN){
                            /*close socket*/
                            epoll_ctl(epfd,EPOLL_CTL_DEL,mysockfd,NULL);
                            close(mysockfd);
                        }
                    }
                    else if (recvlen == 0){
                        epoll_ctl(epfd,EPOLL_CTL_DEL,mysockfd,NULL);
                        close(mysockfd);
                    }
                    else{
                        /*read complete*/
                        printf("server received:%s\n\n",rwEvent->buffer);
                       
                        rwEvent->len    = recvlen;
                        rwEvent->offset = recvlen;
                        
                        //event.data.fd = mysockfd;
                        event.data.ptr = rwEvent;
                        event.events = EPOLLOUT|EPOLLET;
                        epoll_ctl(epfd,EPOLL_CTL_MOD,mysockfd,&event);
                    }
                    
                }
                else if (event_list[n].events & EPOLLOUT){
                    /*write event*/
                    printf("write\n\n");
                    rwEvent = (EVENT_S*)event_list[n].data.ptr;
                    //mysockfd = event_list[n].data.fd;
                    mysockfd = rwEvent->sockfd;
                    if (mysockfd < 0){
                        continue;
                    }
                    
                    send(mysockfd,rwEvent->buffer,rwEvent->len,0);
                    rwEvent->buffer[0] = 0;
                    rwEvent->len       = 0;
                    rwEvent->offset    = 0;
                    
                    //event.data.fd = mysockfd;
                    event.data.ptr = rwEvent;
                    event.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epfd,EPOLL_CTL_MOD,mysockfd,&event);
                }                
            }
        }
    }
    
    return 0;
}

int setup_socket(char *sIP,int nPort,int domain,int type,int protocol)
{
    int listenfd = socket(domain,type,protocol);
    if (listenfd < 0){
        return -1;
    }
    
    int reuse = 1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    
    /*set socket fd as NONBLOCK*/
    setnonblocking(listenfd);
    
    struct sockaddr_in listenAddr;
    
    memset(&listenAddr,0,sizeof(listenAddr));
    
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port   = htons(nPort);
    if(sIP == 0){
        listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else{
        listenAddr.sin_addr.s_addr = inet_addr(sIP);
    }
    
    if(bind(listenfd,(struct sockaddr*)&listenAddr,sizeof(listenAddr)) == -1){
        return -1;
    }
    
    listen(listenfd,4096);
    
    return listenfd;
}

