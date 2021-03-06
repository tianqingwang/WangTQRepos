#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class CSock
{
public:
    CSock(){ /*construct function*/
        m_sockfd = -1;
    };
    ~CSock();/*deconstruct function*/
	/*polymorphism of function Create()*/
    bool Create(int domain=AF_INET, int type=SOCK_STREAM, int protocol=0); /*setup socket*/
    bool Create(char *sIP,const short nPort, int domain=AF_INET, int type=SOCK_STREAM, int protocol=0);
	
    bool SetSockOpt(int level,int optname, const void *optval, int optlen);
    bool Bind(const struct sockaddr *pAddr,int nAddrLen);
    bool Listen(int backlog = 5);
    int  Connect(char *sIP, int nPort);
    int  Connect(const struct sockaddr *serv_addr, int nAddrLen);
    int  Accept(struct sockaddr *addr, int *nAddrLen);
    
    int  Send(int sockfd,const void *pbuf, int nbuf, int flags = 0);
    int  Receive(int sockfd,void *pbuf, int nbuf, int flags=0);
	
    bool Close();
    
    int  GetSocket();
private:
	int m_sockfd;
};

#endif
