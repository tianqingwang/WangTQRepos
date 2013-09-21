#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#incude  <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int socket_create(int domain, int type, int protocol=0);
int socket_setopt(int sockfd,int level,int optname,const void *optval,int optlen);

/*set fd as non-blocking*/
int socket_nonblock(int sockfd);
int socket_bind(int sockfd,struct sockaddr *pAddr,int nAddrLen);
int socket_listen(int sockfd, int backlog=5);

int socket_connect(int sockfd,struct sockaddr *serv_addr,int nAddrLen);
int socket_accept(int sockfd,struct sockaddr *addr, int *nAddrLen);

int socket_send(int sockfd,void *pbuf, int nbuf, int flags = 0);
int socket_recv(int sockfd, void *pbuf, int nbuf, int flags = 0);

int socket_close(int sockfd);

int socket_setup(int nPort);

#endif
