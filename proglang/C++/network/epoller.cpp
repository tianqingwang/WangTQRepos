#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include "epoller.h"

char buf[1024];

//void EventAccept(event_t *ev);

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
    
    /*pre-allocation of connections*/
    loop->connections     = new connection_t[loop->connection_n];
    if (!loop->connections){
        /*todo: write to log*/
        return -1;
    }
    
    /*pre-allocation of read and write events*/
    loop->read_events    = new event_t[loop->connection_n];
    if (!loop->read_events){
        return -1;
    }
    
    /*initialize the read events*/
    rev = loop->read_events;
    for (i=0; i<loop->connection_n; i++){
        rev[i].close = 1;
        rev[i].accept = 1;
    }
    
    loop->write_events   = new event_t[loop->connection_n];
    if (!loop->write_events){
        return -1;
    }
    
    wev = loop->write_events;
    for (i=0; i<loop->connection_n; i++){
        wev[i].close = 1;
        
    }
    
    
    i = loop->connection_n;
    connection_t *c = loop->connections;
    connection_t *next;
    
    next = NULL;
    
    do{
        i--;
        c[i].data = next;
        c[i].fd = -1;
        c[i].read = &loop->read_events[i];
        c[i].write = &loop->write_events[i];
        next = &c[i];
    }while(i);
    
    
    loop->free_connections = next;
    loop->free_connection_n = loop->connection_n;
    
    m_loop = loop;
    
    return 0;
}

void CEpoller::EpollDone(epoller_t *loop)
{
    
    if (loop->read_events){
        delete [] loop->read_events;
        loop->read_events = NULL;
    }
    
    if (loop->write_events){
        delete [] loop->write_events;
        loop->write_events = NULL;
    }
    
    if (loop->connections)
    {
        delete [] loop->connections;
        loop->connections = NULL;
    }
}

int CEpoller::EventTimedWait(epoller_t *loop,int timeout)
{
    int          nevents;
    int          i;
    connection_t *conn;
    event_t      *conn_ev;
    uint32_t     revents;
    event_t      *rev;  /*read event*/
    event_t      *wev;  /*write event*/
    
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
#if 0
        conn    = (connection_t*)m_events_list[i].data.ptr;
        revents = m_events_list[i].events;
        
        if (revents &(EPOLLERR | EPOLLHUP)){
            /*todo: maybe need to log*/
            continue;
        }
       
        rev = conn->read;
        
        if (revents & EPOLLIN){
            if (conn->fd == m_listening_fd){
                //rev->handler = EventAccept;
                printf("push to accept_event_queue\n");
                rev->accept = 1;
                PushEventToQueue(rev,&accept_event_queue);
            }
            else{
                printf("push to read event queue\n");
                PushEventToQueue(rev,&rw_event_queue);
            }
        }
        
        wev = conn->write;
        
        if (revents &EPOLLOUT){
            printf("push to write event queue\n");
            PushEventToQueue(wev,&rw_event_queue);
        }
#endif

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
        #if 0
        if (revents & EPOLLIN){
            if (conn_ev->fd = m_listening_fd){
                printf("accept:conn_ev=%p\n",conn_ev);
                PushEventToQueue(conn_ev,&accept_event_queue);
            }
            else{
                printf("read:conn_ev=%p\n",conn_ev);
                conn_ev->accept = 0;
                PushEventToQueue(conn_ev,&rw_event_queue);
            }
        }
        
        if (revents & EPOLLOUT){
            printf("push to write event queue\n");
            conn_ev->accept = 0;
            PushEventToQueue(conn_ev,&rw_event_queue);
        }
        #endif
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
#if 0
    connection_t *c;
    c = (connection_t*)ev->data;
    printf("eventread: c->fd = %d\n",c->fd);
    buf[0] = 0;
    int readbytes = read(c->fd,buf,1024);
    
    if (readbytes < 0){
        printf("read error\n");
        return;
    }
    
    if (readbytes == 0){
        return;
    }
    
    printf("server recv: %s\n",buf);
#endif
    
    memset(buf,0,sizeof(buf));
    int readbytes = read(ev->fd,buf,1024);
    if (readbytes < 0){
        printf("read error\n");
        return;
    }
    
    if (readbytes == 0){
        return;
    }
    
    printf("server recv: %s\n",buf);
}

void CEpoller::EventWrite(event_t *ev)
{
    connection_t *c;
    c = (connection_t*)ev->data;
    
    
}


int CEpoller::ConnectionAdd(connection_t *c)
{
    struct epoll_event ee;
    
    ee.events   = EPOLLIN | EPOLLET;
    ee.data.ptr = (void*)c;
    
    if (epoll_ctl(m_epfd,EPOLL_CTL_ADD,c->fd,&ee) == -1){
        /*todo: write something to log*/
        return -1;
    }
  
    return 0;
}

int CEpoller::EventAdd(event_t *ev,int event)
{
    int                op;
    struct epoll_event ee;
    event_t            *e;
    connection_t       *c;
    uint32_t           revent;

#if 0    
    c = (connection_t*)ev->data;
    
    if (event & EPOLLIN){
        e = c->write;
        revent = EPOLLOUT;
    }
    else{
        e = c->read;
        revent = EPOLLIN;
    }
    
    if (e->active){
        op = EPOLL_CTL_MOD;
    }
    else{
        op = EPOLL_CTL_ADD;
    }
    
    ee.data.ptr = (void*)c;
    ee.events   = revent;
    
    if (epoll_ctl(m_epfd,op,c->fd,&ee) == -1){
        /*todo: write something to log*/
        return -1;
    }
#endif

    ee.data.ptr = (void*)ev;
    ee.events = event | EPOLLET;
    if (epoll_ctl(m_epfd,EPOLL_CTL_ADD,ev->fd,&ee) == -1){
        return -1;
    }
}

int CEpoller::EventDel(event_t *ev, int event)
{
    connection_t           *c;
    int                    op;
    struct epoll_event     ee;
    
    c = (connection_t*)ev->data;
    
    ev->active = 0;
    
    op = EPOLL_CTL_DEL;
    
    ee.events = 0;
    ee.data.ptr = NULL;
    
    if (epoll_ctl(m_epfd,op,c->fd,&ee) == -1){
        /*todo: log*/
        return -1;
    }
    
    return 0;
}

void CEpoller::EventSetCallback(epoller_t *loop,event_handler_t callback, int flag)
{
    connection_t  *c;
}

void CEpoller::EventAccept(event_t *ev)
{
    struct sockaddr_in  sin;
    int                 port;
    int                 connfd;
    socklen_t           socklen;
    connection_t        *c,*nc;
    struct epoll_event  ee;
#if 0    
    c = (connection_t*)ev->data;
    ev->accept = 0;
    /*EPOLLET mode,accept all connection until errno 
      is EAGAIN.
    */
    printf("do accept\n");
    do{
        socklen = sizeof(sin);
        
        connfd = accept(c->fd,(struct sockaddr*)&sin,&socklen);
       
        if (connfd == -1){
            if (errno == EAGAIN){
                /*all connections are done*/
                printf("c->fd=%d,EAGAIN occurred\n",c->fd);
                return;
            }
            
            /*todo: connection error, write something to log*/
            return;
        }
        
        nc = GetConnection(m_loop,connfd);
        if (nc == NULL){
            /*maybe need to close socket*/
            /*todo: write something to log*/
            printf("GetConnection failed\n");
            return ;
        }

       
        /*set non-blocking*/
        if (SetNonBlock(connfd) == -1){
            /*todo: write something to log*/
            
            /*todo: maybe need to close socket*/
            printf("SetNonBlock failed\n");
            return;
        }        
        nc->fd = connfd;
        
        nc->read->close  = 0;
        nc->read->active = 1;
        nc->write->active = 1;
       
        /*add connection*/
        if (ConnectionAdd(nc) == -1){
            /*todo: write something to log*/
            
            /*close accepted connection*/
            printf("ConnectionAdd failed\n");
            return;
        }
        
        
    }while(1);
#endif
    
    printf("do accept,ev->fd=%d\n",ev->fd);
    ev->accept = 0;
    do{
        socklen = sizeof(sin);
        connfd = accept(ev->fd,(struct sockaddr*)&sin,&socklen);
        if (connfd < 0){
            return;
        }
        printf("connfd = %d\n",connfd);
        SetNonBlock(connfd);
        
        event_t *new_event = new event_t;
        new_event->fd = connfd;
        new_event->accept = 0;
        printf("new_event:%p\n",new_event);
        ee.data.ptr = (void*)new_event;
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

connection_t *CEpoller::GetConnection(epoller_t *loop,int connfd)
{
    connection_t *c;
    event_t      *rev;
    event_t      *wev;
    
    c = loop->free_connections;
    if (c == NULL){
        /*todo: write to log*/
        return NULL;
    }
    
    loop->free_connections = (connection_t*)c->data;
    loop->free_connection_n --;
    
    rev = c->read;
    wev = c->write;
    
    memset(c,0,sizeof(connection_t));
    
    c->read = rev;
    c->write = wev;
    c->fd    = connfd;
    
    rev->data = c;
    wev->data = c;
    
    return c;
    
}

void CEpoller::FreeConnection(epoller_t *loop,connection_t *c)
{
    if (c == NULL){
        return ;
    }
    
    c->data = loop->free_connections;
    loop->free_connections = c;
    loop->free_connection_n ++;
}

void CEpoller::SetListenSocketFD(int fd)
{
    m_listening_fd = fd;
}

