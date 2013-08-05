#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "socket.h"

int Create(int domain, int type, int protocol)
{
    int sockfd = -1;
    sockfd = socket(domain,type,protocol);
#ifdef DEBUG
    printf("(%s:%d):sockfd=%d\n",__FILE__,__LINE__,sockfd);
#endif
    return sockfd;
}

int CreateByIP(char *sIP,short nPort,int domain, int type, int protocol)
{
    struct sockaddr_in sockAddr;
    int sockfd = -1;
    
    sockfd = socket(domain,type,protocol);
    if (sockfd == -1){
        return sockfd;
    }
    
    memset(&sockAddr,0,sizeof(sockAddr));
    sockAddr.sin_family = domain;
    if (sIP == 0){
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else{
        sockAddr.sin_addr.s_addr = inet_addr(sIP);
    }
    sockAddr.sin_port = htons(nPort);
    
    
}

int SetSockOpt(int sockfd,int level,int optname, const void *optval, int optlen)
{
    return setsockopt(sockfd,level,optname,optval,optlen);
}

int Bind(int sockfd,struct sockaddr *pAddr,int nAddrLen)
{
    return bind(sockfd,pAddr,nAddrLen);
}

int Listen(int sockfd,int backlog)
{
    return listen(sockfd,backlog);
}

int Connect(int sockfd,struct sockaddr *serv_addr, int nAddrLen)
{
    return connect(sockfd,serv_addr,nAddrLen);
}

int Accept(int sockfd,struct sockaddr *addr, int *nAddrLen)
{
    return accept(sockfd,addr,(socklen_t*)nAddrLen);
}

int Send(int sockfd,void *pbuf, int nbuf, int flags)
{
    return send(sockfd,pbuf,nbuf,flags);
}

int Receive(int sockfd,void *pbuf, int nbuf, int flags)
{
    return recv(sockfd,pbuf,nbuf,flags);
}

int Close(int sockfd)
{
    return close(sockfd);
}