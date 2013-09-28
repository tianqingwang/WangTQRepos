#include <stdio.h>
#include <fcntl.h>
#include "socket.h"

int socket_create(int domain, int type, int protocol=0)
{
    return socket(domain,type,protocol);
}

int socket_setopt(int sockfd,int level,int optname,const void *optval,int optlen)
{
    return setsockopt(sockfd,level,optname,optval,optlen);
}

int socket_reuseaddr(int sockfd){
    int reuse_on =1;
    return setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse_on,sizeof(reuse_on));
}


int socket_nonblock(int sockfd)
{
    int flags;
    
    flags = fcntl(sockfd,F_GETFL);
    if (flags == -1){
        return -1;
    }
    
    flags |= O_NONBLOCK;
    if (fcntl(sockfd,F_SETFL,flags) == -1){
        return -1;
    }
    
    return 0;
}

int socket_bind(int sockfd,struct sockaddr *pAddr,int nAddrLen)
{
    return bind(sockfd,pAddr,nAddrLen);
}

int socket_listen(int sockfd, int backlog=5)
{
    return listen(sockfd,backlog);
}

int socket_connect(int sockfd,struct sockaddr *serv_addr,int nAddrLen)
{
    return connect(sockfd,serv_addr,nAddrLen);
}

int socket_accept(int sockfd,struct sockaddr *addr, int *nAddrLen)
{
    return accept(sockfd,addr,(socklen_t*)nAddrLen);
}

int socket_send(int sockfd,void *pbuf, int nbuf, int flags = 0)
{
    return send(sockfd,pbuf,nbuf,flags);
}

int socket_recv(int sockfd, void *pbuf, int nbuf, int flags = 0)
{
    return recv(sockfd,pbuf,nbuf,flags);
}

int socket_close(int sockfd)
{
    return close(sockfd);
}

int socket_setup(int nPort)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    
    sockfd = socket_create(AF_INET,SOCK_STREAM,0);
    if (sockfd == -1){
        return -1;
    }
    
    if (socket_nonblock(sockfd) == -1){
        return -1;
    }
    
    if (socket_reuseaddr(sockfd) == -1){
        return -1;
    } 

    memset(serv_addr,0,sizeof(sockaddr_in));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(nPort);    

    if (socket_bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
        return -1;
    }
    
    if (socket_listen(sockfd) == -1){
        return -1;
    }
    
    return sockfd;
}
