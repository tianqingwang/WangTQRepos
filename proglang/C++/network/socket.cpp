#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "socket.h"

/*purpose:create socket only*/
bool CSock::Create(int domain, int type, int protocol)
{
    m_sockfd = socket(domain,type,protocol);
#ifdef DEBUG
    printf("(%s:%d):m_sockfd=%d\n",__FILE__,__LINE__,m_sockfd);
#endif
    return m_sockfd == -1 ? false: true;
}

/*purpose: setup socket descriptor and bind to address.*/
bool CSock::Create(const char *sIP,const short nPort, int domain, int type, int protocol)
{
    struct sockaddr_in sockAddr;
    
    /*get a socket descriptor*/
    m_sockfd = socket(domain,type,protocol);
    if (m_sockfd == -1)
    {
        return false;
    }
	
    /*bind address*/
	memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = domain;
    if(sIP == 0){
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else{
        sockAddr.sin_addr.s_addr = inet_addr(sIP);
    }
    sockAddr.sin_port = htons(nPort);
    
    if (Bind((const struct sockaddr*)&sockAddr,sizeof(sockAddr)) == false){
        CloseSocket();
        return false;
    }
    
    return true;
}

bool CSock::SetSockOpt(int level,int optname, const void *optval, int optlen)
{
    int ret = setsockopt(m_sockfd,level,optname,optval,optlen);
    
    return ret == 0? true:false;
}

bool CSock::Bind(const struct sockaddr *pAddr, int nAddrLen)
{
    int ret = 0;
    ret = bind(m_sockfd,pAddr,nAddrLen);
#ifdef DEBUG
    printf("(%s:%d): bind ret=%d\n",__FILE__,__LINE__,ret);
#endif
    
    return ret == 0 ? true:false;
}

bool CSock::Listen(int backlog){
    int ret = 0;
    ret = listen(m_sockfd,backlog);

#ifdef DEBUG
    printf("(%s:%d):listen ret=%d\n",__FILE__,__LINE__,ret);
#endif
    
    return ret == 0 ? true:false;
}

bool CSock::Connect(const struct sockaddr *serv_addr, int nAddrLen)
{
    return connect(m_sockfd,serv_addr,nAddrLen);
}

bool CSock::Accept(struct sockaddr *addr, int *nAddrLen)
{
    return accept(m_sockfd,addr, (socklen_t*)nAddrLen);
}

int CSock::Send(const void *pbuf, int nbuf, int flags)
{
    return send(m_sockfd,pbuf,nbuf,flags);
}

int CSock::Receive(void *pbuf, int nbuf, int flags)
{
    return recv(m_sockfd,pbuf,nbuf,flags);
}

bool CSock::CloseSocket()
{
    return close(m_sockfd);
}