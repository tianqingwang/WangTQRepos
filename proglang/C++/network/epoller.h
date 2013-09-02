#ifndef __C_EPOLLER_H__
#define __C_EPOLLER_H__

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/epoll.h>
#include "socket.h"

#define EPOLL_FD_MAXSIZE       (102400)
#define EPOLL_EVENTS_MAXSIZE   (10240)
#define EPOLL_TIMEOUT_MS       (10)

#define BUFSIZE                (1024)

typedef struct event_s{
    int sockfd;
    char buffer[BUFSIZE];
    int  offset;
    int  len;
}event_t;

class CEpoller
{
public:
    CEpoller();
    ~CEpoller();
    
    int Init(int epoll_size, int epoll_event_size,int timeout);
    int AddEpollIO(int fd,unsigned int flag);
    int ModEpollIO(int fd,unsigned int flag);
    int DelEpollIO(int fd,unsigned int flag);
    void AttachSocket(CSock *socket);
    void DetachSocket();
    int EventLoop();
    int HandleAccept();
    int HandleRead();
    int HandleWrite();
private:
    int m_epfd;  /*epoll descriptor*/
    int m_timeout;
    int m_epollsize;
    int m_epolleventsize;
    struct epoll_event *m_events;
    int m_sockfd;
};

#endif
