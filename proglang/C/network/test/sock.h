#ifndef __SOCK_H__
#define __SOCK_H__

int new_socket(char *servIP,int port);
int socket_nonblock(int fd);
int socket_write(int sockfd,const void *vbuf, size_t len);
int socket_read(int sockfd, void *vbuf, size_t len);
int socket_close();

#endif
