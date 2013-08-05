#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#incude  <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int Create(int domain=AF_INET, int type=SOCK_STREAM, int protocol=0); /*setup socket*/
/*create sock and bind to sIP:nPort*/
int CreateByIP(char *sIP,short nPort,int domain=AF_INET, int type=SOCK_STREAM, int protocol=0);
/*set socket options*/
int SetSockOpt(int sockfd,int level,int optname, const void *optval, int optlen);

int Bind(int sockfd,struct sockaddr *pAddr,int nAddrLen);
int Listen(int sockfd,int backlog=5);
int Connect(int sockfd,struct sockaddr *serv_addr, int nAddrLen);
int Accept(int sockfd,struct sockaddr *addr, int *nAddrLen);
/*data operation*/
int Send(int sockfd,void *pbuf, int nbuf, int flags = 0);
int Receive(int sockfd,void *pbuf, int nbuf, int flags=0);

int Close(int sockfd);

#endif
