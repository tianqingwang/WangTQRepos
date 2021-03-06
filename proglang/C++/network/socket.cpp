#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "socket.h"


CSock::~CSock()
{
    if (m_sockfd != 0){
        Close();
    }
}

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
bool CSock::Create(char *sIP,const short nPort, int domain, int type, int protocol)
{
    struct sockaddr_in sockAddr;
    /*get a socket descriptor*/
    m_sockfd = socket(domain,type,protocol);
#ifdef DEBUG
    printf("(%s:%d):m_sockfd=%d\n",__FILE__,__LINE__,m_sockfd);
#endif
    if (m_sockfd == -1)
    {
        return false;
    }
	
    /*bind address*/
	memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = domain;
    if(sIP == 0){
#ifdef DEBUG
        printf("(%s:%d):sIP=0, set to INADDR_ANY\n",__FILE__,__LINE__);
#endif
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else{
        sockAddr.sin_addr.s_addr = inet_addr(sIP);
    }
    sockAddr.sin_port = htons(nPort);
    
    if (Bind((const struct sockaddr*)&sockAddr,sizeof(sockAddr)) == false){
#ifdef DEBUG
        printf("(%s:%d):bind failed\n",__FILE__,__LINE__);
#endif
        Close();
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

int CSock::Connect(char *sIP, int nPort)
{
    int nret;
    
    struct sockaddr_in addr;
    int  nlen = sizeof(addr);
    
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(nPort);
    inet_pton(AF_INET,sIP,&addr.sin_addr);
    
    nret = connect(m_sockfd,(struct sockaddr*)&addr,nlen);
    
    return nret;
}

int  CSock::Connect(const struct sockaddr *serv_addr, int nAddrLen)
{
    int ret = 0;
    ret = connect(m_sockfd,serv_addr,nAddrLen);
#ifdef DEBUG
    printf("(%s:%d):connect ret=%d\n",__FILE__,__LINE__,ret);
#endif
    return ret;
}

int  CSock::Accept(struct sockaddr *addr, int *nAddrLen)
{
    int ret = 0;
    ret = accept(m_sockfd,addr, (socklen_t*)nAddrLen);

#ifdef DEBUG
    printf("(%s:%d): accept ret=%d\n",__FILE__,__LINE__,ret);
#endif
    return ret;
}

int CSock::Send(int sockfd,const void *pbuf, int nbuf, int flags)
{
    return send(sockfd,pbuf,nbuf,flags);
}

int CSock::Receive(int sockfd,void *pbuf, int nbuf, int flags)
{
    return recv(sockfd,pbuf,nbuf,flags);
}

bool CSock::Close()
{
    return close(m_sockfd);
}

int CSock::GetSocket()
{
    return m_sockfd;
}