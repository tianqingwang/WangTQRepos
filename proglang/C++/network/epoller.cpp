#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include "epoller.h"

#define THREAD_OPEN   0

char buf[1024];

CEpoller::CEpoller()
    :m_epfd(-1)
    ,m_timeout(EPOLL_TIMEOUT_MS)
    ,m_epollsize(EPOLL_FD_MAXSIZE)
    ,m_epolleventsize(EPOLL_EVENTS_MAXSIZE)
#if 0
    ,m_events(NULL)
#endif
    ,m_events_list(NULL)
    ,accept_event_queue(NULL)
    ,rw_event_queue(NULL)
{
#if THREAD_OPEN
    /*create read/write thread*/
    pthread_create(&m_threadID,NULL,&CEpoller::workThread,(void*)this);
#endif
}

CEpoller::~CEpoller()
{
    if (m_epfd != -1){
        close(m_epfd);
        m_epfd = -1;
    }

#if 0    
    if (m_events){
        delete [] m_events;
        m_events = NULL;
    }
#endif
    
    if (m_events_list){
        delete [] m_events_list;
        m_events_list = NULL;
    }
    
    

#if THREAD_OPEN    
    pthread_join(m_threadID,NULL);
#endif
}

int CEpoller::EpollInit(epoller_t *loop)
{
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
    
    loop->write_events   = new event_t[loop->connection_n];
    if (!loop->write_events){
        return -1;
    }
    
    
    int i = loop->connection_n;
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
        conn    = (connection_t*)m_events_list[i].data.ptr;
        revents = m_events_list[i].events;
        
        if (revents &(EPOLLERR | EPOLLHUP)){
            /*todo: maybe need to log*/
        }
        
        if ((revents & (EPOLLERR | EPOLLHUP)) && (revents &(EPOLLIN|EPOLLOUT)) == 0)
        {
            /*???*/
            revents |= EPOLLIN|EPOLLOUT;
        }
        
        rev = conn->read;
        
        if (revents & EPOLLIN){
            if (rev->accept){
                PushEventToQueue(rev,accept_event_queue);
            }
            else{
                PushEventToQueue(rev,rw_event_queue);
            }
        }
        
        wev = conn->write;
        
        if (revents &EPOLLOUT){
            PushEventToQueue(wev,rw_event_queue);
        }
    }
    
    /*todo: add unlock for multi-process or multi-threads*/
    
    return 0;
}

void CEpoller::EventProcess(epoller_t *loop, event_t *queue)
{
    event_t *event;
    
    for(;;){
        event = queue;
        if (event == NULL){
            return;
        }
        
        /*delete event from queue*/
        queue = queue->next;
        event->next = NULL;
        
        if (queue){
            queue->prev = NULL;
        }
        
        event->handler(event);
    }
}

int CEpoller::ConnectionAdd(connection_t *c)
{
    struct epoll_event ee;
    
    ee.events   = EPOLLIN|EPOLLOUT|EPOLLET;
    ee.data.ptr = (void*)c;
    
    if (epoll_ctl(m_epfd,EPOLL_CTL_ADD,c->fd,&ee) == -1){
        /*todo: write something to log*/
        return -1;
    }
 
#if 0 
    c->read->active  = 1;
    c->write->active = 1;
#endif    
    return 0;
}

int CEpoller::EventAdd(event_t *ev,int event)
{
    int                op;
    struct epoll_event ee;
    event_t            *e;
    connection_t       *c;
    uint32_t           revent;
    
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

#if 0    
    c = loop->connections;
    
    if (flag & EPOLL_READ_FLAG){
        c->read->handler = callback;
    }
    else if (flag & EPOLL_WRITE_FLAG){
        c->write->handler = callback;
    }
#endif
}

void CEpoller::EventAccept(event_t *ev)
{
    struct sockaddr_in  sin;
    int                 port;
    int                 connfd;
    socklen_t           socklen;
    connection_t        *c,*nc;
    
    c = (connection_t*)ev->data;
    
    /*EPOLLET mode,accept all connection until errno 
      is EAGAIN.
    */
    do{
        socklen = sizeof(sin);
        connfd = accept(c->fd,(struct sockaddr*)&sin,&socklen);
        
        if (connfd == -1){
            if (errno == EAGAIN){
                /*all connections are done*/
                return;
            }
            
            /*todo: connection error, write something to log*/
            return;
        }
        
        nc = new connection_t;
        if (nc == NULL){
            /*maybe need to close socket*/
            /*todo: write something to log*/
            return ;
        }
        
        /*set non-blocking*/
        if (SetNonBlock(connfd) == -1){
            /*todo: write something to log*/
            
            /*todo: maybe need to close socket*/
            
            return;
        }
        
        nc->fd = connfd;
        nc->read = c->read;
        nc->write = c->write;
        
        /*add connection*/
        if (ConnectionAdd(nc) == -1){
            /*todo: write something to log*/
            
            /*close accepted connection*/
            
            return;
        }
        
    }while(1);
}

int CEpoller::SetNonBlock(int fd)
{
    int flag;
    
    if (fcntl(fd,F_GETFL,flag) == -1){
        return -1;
    }
    
    flag |= O_NONBLOCK;
    
    if (fcntl(fd,F_SETFL,flag) == -1){
        return -1;
    }
    
    return 0;
}


void CEpoller::EventLoop(epoller_t *loop)
{
    for(;;){
        
        EventTimedWait(loop,m_timeout);
        
        if (accept_event_queue){
            EventProcess(loop,accept_event_queue);
        }
        
        if (rw_event_queue){
            EventProcess(loop,rw_event_queue);
        }
    }
}

#if 0
int CEpoller::Init(int epoll_size, int epoll_event_size,int timeout)
{
    
    m_epollsize = epoll_size;
    m_epolleventsize = epoll_event_size;
    m_timeout   = timeout;
    /*the size field is ignored since kernel version 2.6.8*/
    m_epfd = epoll_create(epoll_size);
    if (m_epfd == -1){
        return -1;
    }
    
    m_events = new struct epoll_event[m_epolleventsize];
    
    if (m_events == NULL){
        return -1;
    }
    
    return 0;
}

int CEpoller::AddEpollIO(int fd, unsigned int flag)
{
    struct epoll_event ev;
    int ctrl_flag;
    
    memset((void*)&ev,0,sizeof(ev));
    
    /*set non-blocking*/
    fcntl(fd,F_GETFL,ctrl_flag);
    ctrl_flag |= O_NONBLOCK;
    fcntl(fd,F_SETFL,ctrl_flag);
    
    ev.data.fd = fd;
    ev.events  = flag;
    
    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD,fd,&ev) < 0){
        return -1;
    }
    
    return 0;
}

int CEpoller::ModEpollIO(int fd, unsigned int flag)
{
    struct epoll_event ev;
    
    memset((void*)&ev,0,sizeof(ev));
    
    ev.data.fd = fd;
    ev.events  = flag;
    
    if (epoll_ctl(m_epfd, EPOLL_CTL_MOD,fd,&ev) < 0){
        return -1;
    }
    
    return 0;
}

int CEpoller::DelEpollIO(int fd, unsigned int flag)
{
    struct epoll_event ev;
    
    memset((void*)&ev,0,sizeof(ev));
    
    ev.data.fd = fd;
    ev.events  = flag;
    
    if (epoll_ctl(m_epfd, EPOLL_CTL_DEL,fd,&ev) < 0){
        return -1;
    }
    
    return 0;
}

int CEpoller::EventLoop()
{
    int sockfd;
    int nfds;
    int i;
    
    time_t timep;
    struct tm *p_tm;
    
    AddEpollIO(m_sockfd,EPOLLIN);/*accept uses default LT mode*/
    
    for(;;){
        nfds = epoll_wait(m_epfd,m_events,m_epolleventsize,m_timeout);
        
        if (nfds < 0){
            if (errno == EINTR){
                continue;
            }
            
            return -1;
        }
         
        for (i=0; i<nfds; i++){
            
            if (m_events[i].events & (EPOLLERR | EPOLLHUP)){
                close(m_events[i].data.fd);
                DelEpollIO(m_events[i].data.fd,0);
                continue;
            }
            
            if (m_events[i].data.fd == m_sockfd){
                /*accept*/
                
                int retval = HandleAccept();
                if (retval > 0){
                    printf("accept %d\n",retval);
                    AddEpollIO(retval,EPOLLIN|EPOLLET);
                }
            }
            else{
                
#if 0
                /*handle read or write*/
                if (HandleReadWrite(m_events[i].data.fd,m_events[i].events) < 0){
                    continue;
                }
#endif  
#if THREAD_OPEN 
                if (m_events[i].events & EPOLLIN){
                    printf("read events\n");
                    m_list.push_back(m_events[i]);
                    
                }
                else if (m_events[i].events & EPOLLOUT){
                    printf("write events\n");
                    m_list.push_back(m_events[i]);
                    
                }
#endif                
            }
            
        }

    }
}

void CEpoller::AttachSocket(CSock *socket)
{
    if (socket == NULL){
        m_sockfd = -1;
        return;
    }
    
    m_sockfd = socket->GetSocket();
}

void CEpoller::DetachSocket()
{
    if (m_sockfd != -1){
        DelEpollIO(m_sockfd,0);
    }
}

int CEpoller::HandleAccept()
{
    int connfd;
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    
    connfd = accept(m_sockfd,(struct sockaddr*)&clientaddr,&clientlen);
    
    return connfd;
}

int CEpoller::HandleReadWrite(int sockfd,int events)
{
    if (sockfd < 0){
        return -1;
    }
    
    if (events & EPOLLIN){
        /*read events*/
        int read_bytes;
        /*to do: need buffer queue to read all data if 
          the sent size by client is larger than the size of received buffer.
        */
        read_bytes = read(sockfd,buf,1024);
        if (read_bytes < 0){
            if (errno != EAGAIN){
                close(sockfd);
                DelEpollIO(sockfd,0);
            }
        }
        else if (read_bytes == 0){
            close(sockfd);
            DelEpollIO(sockfd,0);
        }
        else{
            buf[read_bytes] = '\0';
            printf("received:%s\n",buf);
        }
    }
    else if (events & EPOLLOUT){
        /*write events*/
        char writebuf[]="server said: I was sent!";
        int write_bytes;
        write_bytes = write(sockfd,writebuf,strlen(writebuf));
        if (write_bytes < 0){
            if (errno != EAGAIN){
                close(sockfd);
                DelEpollIO(sockfd,0);
            }
        }
        
    }
}

#if THREAD_OPEN
void *CEpoller::workThread(void *args)
{
    CEpoller *pEpoller = (CEpoller*)args;
    while(1){
        if (pEpoller->m_list.empty()){
            continue;
        }
        
        struct epoll_event ev = pEpoller->m_list.front();
        pEpoller->m_list.pop_front();
        
        
        
        pEpoller->HandleReadWrite(ev.data.fd,ev.events);
    }
}
#endif
#endif

