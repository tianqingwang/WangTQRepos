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

#define MAXEVENTS    (200)

static struct epoll_event *event_list;

int main(int argc, char* argv[]){
    struct epoll_event event;
    int mysockfd = 0;
    int nevents = 0;
    int n = 0;
    char line[1024]={0};
     
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
    
    event.data.fd = listenfd;
    event.events = EPOLLIN|EPOLLET; /*read and edge trigger*/
    
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD,listenfd,&event);
    if (ret == -1){
        perror("epoll_ctl error");
        return -1;
    }
    
    while(1){
        nevents = epoll_wait(epfd, event_list,MAXEVENTS,500);
        for (n=0; n<nevents; n++){
            if (event_list[n].events & (EPOLLERR | EPOLLHUP)){
                fprintf(stderr,"epoll wait error in event_list[%d]\n",n);
                close(event_list[n].data.fd);
                continue;
            }
            else{
                if (event_list[n].data.fd == listenfd){
                    /*accept*/
                    struct sockaddr_in conn_addr;
                    int len = sizeof(conn_addr);
                    int connfd = accept(listenfd,(struct sockaddr*)&conn_addr,(socklen_t*)&len);
                    /*set connfd as O_NONBLOCK*/
                    int flag;
                    fcntl(connfd,F_GETFL,&flag);
                    flag |= O_NONBLOCK;
                    fcntl(connfd,F_SETFL,flag);
                    
                    if (connfd > 0){
                        event.data.fd = connfd;
                        event.events = EPOLLIN|EPOLLET;
                        epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&event);
                    }
                }
                else if (event_list[n].events & EPOLLIN){
                    /*read event*/
                    mysockfd = event_list[n].data.fd;
                    if (mysockfd < 0){
                        continue;
                    }
                    int read_bytes = read(mysockfd,line,1024);
                    /* to do, for edge trigger.
                    while(1){
                        read_bytes = read(event_list[n].data.fd,line,1024);
                        
                    }
                    */
                    
                    if (read_bytes < 0){
                        /**/
                        if (errno != EAGAIN){
                            /*read all data.*/
                            close(mysockfd);
                            event_list[n].data.fd = -1;
                        }
                    }
                    else if (read_bytes == 0){
                        /*end of file*/
                        close(mysockfd);
                        event_list[n].data.fd = -1;
                    }
                    else{
                        line[read_bytes] = '\0';
                        printf("send:%s\n",line);
                        event.data.fd = mysockfd;
                        event.events = EPOLLOUT|EPOLLET;
                        epoll_ctl(epfd,EPOLL_CTL_MOD,mysockfd,&event);
                    }
                }
                else if (event_list[n].events & EPOLLOUT){
                    /*write event*/
                    mysockfd = event_list[n].data.fd;
                    if (mysockfd < 0){
                        continue;
                    }
                    write(mysockfd,line,strlen(line));
                    event.data.fd = mysockfd;
                    event.events = EPOLLIN|EPOLLET;
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
    int flag;
    fcntl(listenfd,F_GETFL,&flag);
    flag |= O_NONBLOCK;
    fcntl(listenfd,F_SETFL,&flag);
    
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