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

enum EVENT_ACTION{
    EVENT_READ = 0,
    EVENT_WRITE,
    EVENT_DELETE
};

struct event{
    void (*callback)(int sockfd, short events, void *args);
    short ev_events;
    void *ev_args;
};

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
    void EventSetCallback(struct event *ev, int sockfd, void (*callback)(int,short, void *),void *args);
    int HandleAccept();
    int HandleReadWrite(int sockfd,int events);
private:
    int m_epfd;  /*epoll descriptor*/
    int m_timeout;
    int m_epollsize;
    int m_epolleventsize;
    struct epoll_event *m_events;
    int m_sockfd;
};

#endif
