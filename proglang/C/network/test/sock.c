#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sock.h"

int new_socket(char *servIP,int port)
{
    int sockfd = -1;
    struct sockaddr_in servaddr;
    
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd < 0){
        socket_close(sockfd);
        return -1;
    }
    
#if 0
    /*non-block*/
    if (socket_nonblock(sockfd) < 0){
        return -1;
    }
#endif
    
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);
    inet_pton(AF_INET,servIP,&servaddr.sin_addr);
    if (connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0){
        return -1;
    }
    
    return sockfd;
}

int socket_nonblock(int fd)
{
    int flags;
    
    flags = fcntl(fd,F_GETFL);
    if (flags < 0){
        return -1;
    }
    
    flags |= O_NONBLOCK;
    
    if (fcntl(fd,F_SETFL,flags) < 0){
        return -1;
    }
    
    return 0;
}

int socket_write(int sockfd,const void *vbuf, size_t len)
{
    int         type;
    size_t      n;
    size_t      w;
    const char *buf;
    
    buf = vbuf;
    n   = len;
    //pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type);
    while(n > 0){
        if ((w=write(sockfd,buf,n)) < 0){
            if (errno == EINTR){
                w = 0;
            }
            else{
                return -1;
            }
        }
        
        n   -= w;
        buf += w;
    }
    //pthread_setcanceltype(type,NULL);
    //pthread_testcancel();
    
    return len;
}

int socket_read(int sockfd, void *vbuf, size_t len)
{
    int type;
    
    size_t      n;
    size_t      r;
    char       *buf;
    
    buf = vbuf;
    n   = len;
    //pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type);
    while(n > 0){
        r = read(sockfd,buf,n);
        if (r < 0){
            if (errno == EINTR){
                r = 0;
            }
            else{
                return -1;
            }
        }
        else if (r == 0){
            break;
        }
        
        n   -= r;
        buf += r;
    }
    //pthread_setcanceltype(type,NULL);
    //pthread_testcancel();
    
    return (len - n);
}

int socket_close(int sockfd)
{
    if (sockfd != -1){
        close(sockfd);
    }
}
