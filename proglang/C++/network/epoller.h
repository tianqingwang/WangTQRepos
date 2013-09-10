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
typedef struct epoller_s        epoller_t;

typedef void (*event_handler_t)(event_t *ev); /*callback prototype*/

struct event_s{ /*r/w events*/
    int               fd;
    void              *data;
//    int               accept;
//    int               events;
//    int               active;
//    int               close;
//    event_handler_t   handler;
    
    event_t           *next;
};

struct epoller_s{
    int               connection_n;
    int               event_size;
    event_t          *rwEvents;
    event_t          *free_events;
    int               free_event_n;
};

class CEpoller
{
public:
    CEpoller();
    ~CEpoller();
    
    /*inline*/
    inline void PushEventToQueue(event_t *ev, event_t **queue){
        if (ev != NULL){
            ev->next = *queue;
            *queue = ev;
        }
    }
    
    int  EpollInit(epoller_t *loop);
    void EpollDone(epoller_t *loop);
    int  EventTimedWait(epoller_t *loop,int timeout);
    void EventProcess(epoller_t *loop, event_t **queue);
    void EventLoop(epoller_t *loop);
    
    int  EventAdd(event_t *ev,int event);
    int  EventDel(event_t *ev,int event);
    
    void EventAccept(event_t *ev);
    void EventRead(event_t *ev);
    void EventWrite(event_t *ev);
    
    int  SetNonBlock(int fd);
    void SetListenSocketFD(int fd);
    
    event_t *GetNewEventBuff(epoller_t *loop);
    void     FreeEventBuff(epoller_t *loop,event_t *ev);

public:
    struct epoll_event *m_events_list;
private:
    int m_epfd;  /*epoll descriptor*/
    int m_listening_fd;
    int m_timeout;
    int m_epollsize;
    int m_epolleventsize;
    
    epoller_t  *m_loop;
    
    event_t *accept_event_queue;
    event_t *rw_event_queue;
};

#endif
