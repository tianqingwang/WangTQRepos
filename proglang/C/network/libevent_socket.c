#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "libevent_socket.h"

int set_socket_nonblock(int fd)
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

int set_socket_reusable(int fd)
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
