#ifndef __C_EPOLLER_H__
#define __C_EPOLLER_H__

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/epoll.h>
#include "socket.h"
#include <list>

using namespace std;

#define EPOLL_FD_MAXSIZE       (102400)
#define EPOLL_EVENTS_MAXSIZE   (10240)
#define EPOLL_TIMEOUT_MS       (10)

#define EPOLL_ACCEPT_FLAG    0x01
#define EPOLL_READ_FLAG      0x02
#define EPOLL_WRITE_FLAG     0x04


#define BUFSIZE                (1024)

typedef struct event_s          event_t;
typedef struct connection_s     connection_t;
typedef struct epoller_s        epoller_t;

typedef void (*event_handler_t)(event_t *ev); /*callback prototype*/

struct event_s{ /*r/w events*/
    void              *data;
    int               fd;
    bool              accept;
    int               events;
    int               active;
    event_handler_t   handler;
    
    /*double direction*/
    event_t           *next;
    event_t           *prev;
};

struct connection_s{
    int         fd;
    void       *data;
    event_t    *read;
    event_t    *write;
};

struct epoller_s{
    int               connection_n;
    int               event_size;
    connection_t     *connections; /*all connections array*/
    connection_t     *free_connections; /*connections that can be used*/
    int               free_connection_n; /*number of connections that can be used*/
    
    event_t          *read_events; /* all read events array*/
    event_t          *write_events;/* all write events array*/
};


#if 0
struct event{
    int sockfd;
    void (*callback)(int sockfd, short events, void *args);
    short ev_events;
    void *ev_args;
};
#endif

class CEpoller
{
public:
    CEpoller();
    ~CEpoller();
    
    /*inline*/
    inline void PushEventToQueue(event_t *ev, event_t *queue){
        if (ev != NULL){
            ev->next = queue;
            queue->prev = ev;
            queue = ev;
        }
    }
    
    int  EpollInit(epoller_t *loop);
    void EpollDone(epoller_t *loop);
    int  EventTimedWait(epoller_t *loop,int timeout);
    void EventProcess(epoller_t *loop, event_t *queue);
    void EventLoop(epoller_t *loop);
    
    int  ConnectionAdd(connection_t *c);
    int  ConnectionDel(connection_t *c);
    
    int  EventAdd(event_t *ev,int event);
    int  EventDel(event_t *ev,int event);
    
    void EventAccept(event_t *ev);
    
    void EventSetCallback(epoller_t *loop,event_handler_t callback, int flag);
    
    int  SetNonBlock(int fd);
#if 0    
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
    static void *workThread(void *args);
#endif
public:
#if 0
    struct epoll_event *m_events;
#endif
    struct epoll_event *m_events_list;
private:
    int m_epfd;  /*epoll descriptor*/
    
    int m_timeout;
    int m_epollsize;
    int m_epolleventsize;
    
    event_t *accept_event_queue;
    event_t *rw_event_queue;
 
#if 0 
    int m_sockfd;
    list<struct epoll_event> m_list;
    pthread_t m_threadID;
#endif
};

#endif
