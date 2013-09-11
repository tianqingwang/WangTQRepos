#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include "epoller.h"


CEpoller::CEpoller()
    :m_epfd(-1)
    ,m_timeout(EPOLL_TIMEOUT_MS)
    ,m_epollsize(EPOLL_FD_MAXSIZE)
    ,m_epolleventsize(EPOLL_EVENTS_MAXSIZE)
    ,m_loop(NULL)
    ,m_events_list(NULL)
    ,accept_event_queue(NULL)
    ,rw_event_queue(NULL)
{
}

CEpoller::~CEpoller()
{
    if (m_epfd != -1){
        close(m_epfd);
        m_epfd = -1;
    }
    
    if (m_events_list){
        delete [] m_events_list;
        m_events_list = NULL;
    }
    
    if (m_loop){
        m_loop= NULL;
    }
    
}

int CEpoller::EpollInit(epoller_t *loop)
{
    event_t *rev;
    event_t *wev;
    
    event_t *ev;
    int      i;

    if (m_epfd == -1){
        m_epfd = epoll_create(loop->connection_n/2);
        
        if (m_epfd == -1){
            
            return -1;
        }
        
        m_epollsize = loop->connection_n/2;
    }
    
    /*allocate m_events_list*/
    if (m_events_list != NULL){
        delete [] m_events_list;
    }
    
    m_events_list = new struct epoll_event[loop->event_size];
    if (m_events_list == NULL){
        return -1;
    }
    
    m_epolleventsize = loop->event_size;

    loop->rwEvents = new event_t[loop->connection_n];
    if (loop->rwEvents == NULL){
        return -1;
    }
    
    i = loop->connection_n;
    event_t *next;
    next = NULL;
    event_t *e = loop->rwEvents;
    
    do{
        i--;
        e[i].data = next;
        e[i].fd   = -1;
        next = &e[i];
    }while(i);
    
    loop->free_events = next;
    loop->free_event_n = loop->connection_n;
    
    m_loop = loop;
   
    file_id = open("black_recv.bmp",O_CREAT|O_RDWR);    
 
    return 0;
}

void CEpoller::EpollDone(epoller_t *loop)
{
    if (loop->rwEvents){
        delete[] loop->rwEvents;
        loop->rwEvents = NULL;
    }
    
    loop->free_events = NULL;
}

int CEpoller::EventTimedWait(epoller_t *loop,int timeout)
{
    int          nevents;
    int          i;
    event_t      *conn_ev;
    uint32_t     revents;
    
    nevents = epoll_wait(m_epfd,m_events_list,m_epolleventsize,timeout);
    
    if (nevents == -1){
        /*it was interrupted within the duration of timeout*/
        if (errno == EINTR){
            return 0;
        }
        else{
            return -1;
        }
    }
    
    if (nevents == 0){
        return -1;
    }
    
    /*todo: add lock for multi-process or multi-threads*/
    
    printf("nevents=%d\n",nevents);
    
    for (i=0; i<nevents; i++){
        conn_ev = (event_t*)m_events_list[i].data.ptr;
        revents = m_events_list[i].events;
        if (revents &(EPOLLERR | EPOLLHUP)){
            continue;
        }
        
        if (conn_ev->fd == m_listening_fd){
            PushEventToQueue(conn_ev,&accept_event_queue);
        }
        else{
            PushEventToQueue(conn_ev,&rw_event_queue);
        }
        
    }
        
    /*todo: add unlock for multi-process or multi-threads*/
    
    return 0;
}

void CEpoller::EventProcess(epoller_t *loop, event_t **queue)
{
    event_t *event;
    
    printf("queue address=%p\n",*queue);
    
    for(;;){
        event = *queue;
        if (event == NULL){
            printf("event == NULL now\n");
            return;
        }
        
        /*delete event from queue*/
        printf("event=%p\n",event);
        *queue = event->next;
        
//        event->handler(event);
        if (event->fd == m_listening_fd){
            EventAccept(event);
        }
        else{
            EventRead(event);
        }
    }
}

void CEpoller::EventRead(event_t *ev)
{
    char buf[BUFSIZE]={0};
    int readbytes;
    
    do{    
        readbytes = read(ev->fd,buf,BUFSIZE);
        if (readbytes < 0){
            if (errno == EAGAIN){
                return;
            }
            printf("read error\n");
            return;
        }
    
        if (readbytes == 0){
            printf("read bytes = 0\n");
             return;
        }
        if(readbytes < BUFSIZE){
            buf[readbytes]='\0';
            write(file_id,buf,readbytes);
            break;
        }
        if (readbytes == BUFSIZE){
            buf[readbytes]='\0';
            write(file_id,buf,readbytes);
            continue;
        }
        
        
    }while(1);
}

void CEpoller::EventWrite(event_t *ev)
{   
    char sendbuf[]="echo words!\n";
    int writebytes;
    
    writebytes = write(ev->fd,sendbuf,strlen(sendbuf));
    if (writebytes < 0){
        if (errno == EAGAIN){
            return;
        }
    }
    else{
        /*correct*/
        
    }
}

int CEpoller::EventAdd(event_t *ev,int event)
{
    struct epoll_event ee;
    
    ee.data.ptr = (void*)ev;
    ee.events = event | EPOLLET;
    if (epoll_ctl(m_epfd,EPOLL_CTL_ADD,ev->fd,&ee) == -1){
        /*todo: write to log*/
        return -1;
    }
    
    return 0;
}

int CEpoller::EventDel(event_t *ev, int event)
{
    struct epoll_event     ee;
    
    ee.events = 0;
    ee.data.ptr = NULL;
    
    if (epoll_ctl(m_epfd,EPOLL_CTL_DEL,ev->fd,&ee) == -1){
        /*todo: log*/
        return -1;
    }
    
    return 0;
}


void CEpoller::EventAccept(event_t *ev)
{
    struct sockaddr_in  sin;
    int                 port;
    int                 connfd;
    socklen_t           socklen;
    struct epoll_event  ee;

    printf("do accept,ev->fd=%d\n",ev->fd);
    
    do{
        socklen = sizeof(sin);
        connfd = accept(ev->fd,(struct sockaddr*)&sin,&socklen);
        if (connfd < 0){
            return;
        }
        printf("connfd = %d\n",connfd);
        SetNonBlock(connfd);
        
        event_t *ev = GetNewEventBuff(m_loop);
        ev->fd = connfd;
        ee.data.ptr = (void*)ev;
        ee.events   = EPOLLIN | EPOLLET;
        epoll_ctl(m_epfd,EPOLL_CTL_ADD,connfd,&ee);
        
    }while(1);

}

int CEpoller::SetNonBlock(int fd)
{
    int flags;
    
    if ((flags = fcntl(fd,F_GETFL,NULL)) < 0){
        /*todo: write to log*/
        return -1;
    }
    
    if (fcntl(fd,F_SETFL,flags|O_NONBLOCK) == -1){
        /*todo: write to log*/
        return -1;
    }
    
    return 0;
}


void CEpoller::EventLoop(epoller_t *loop)
{
    for(;;){
        
        EventTimedWait(loop,m_timeout);
        
        if (accept_event_queue){
            
            EventProcess(loop,&accept_event_queue);
        }
        
        if (rw_event_queue){
            EventProcess(loop,&rw_event_queue);
        }
    }
}

event_t  *CEpoller::GetNewEventBuff(epoller_t *loop)
{
    event_t *ev;
    
    ev = loop->free_events;
    if (!ev){
        return NULL;
    }
    
    loop->free_events = (event_t*)ev->data;
    loop->free_event_n --;
    
    return ev;
}

void CEpoller::FreeEventBuff(epoller_t *loop,event_t *ev)
{
    if (!ev){
        return;
    }
    
    ev->data = loop->free_events;
    loop->free_events = ev;
    loop->free_event_n++;
}


void CEpoller::SetListenSocketFD(int fd)
{
    m_listening_fd = fd;
}

