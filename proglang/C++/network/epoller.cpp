#include <string.h>
#include "epoller.h"

CEpoller::CEpoller()
    :m_epfd(-1)
    ,m_timeout(EPOLL_TIMEOUT_MS)
    ,m_epollsize(EPOLL_FD_MAXSIZE)
    ,m_epolleventsize(EPOLL_EVENTS_MAXSIZE)
    ,m_events(NULL)
{
}

CEpoller::~CEpoller()
{
    if (m_epfd != -1){
        close(m_epfd);
        m_epfd = -1;
    }
    
    if (m_events){
        delete [] m_events;
        m_events = NULL;
    }
}

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
    
    memset((void*)&ev,0,sizeof(ev));
    
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

int CEpoller::EventLoop(int listenfd)
{
    int sockfd;
    int nfds;
    int i;
    
    AddEpollIO(listenfd,EPOLLIN);
    
    for(;;){
        nfds = epoll_wait(m_epfd,m_events,m_epolleventsize,m_timeout);
        if (nfds < 0){
            if (errno == EINTR){
                continue;
            }
            
            return -1;
        }
#if 0        
        for (i=0; i<nfds; i++){
            if (m_events[i].events & (EPOLLERR | EPOLLHUP)){
                close(m_events[i].data.fd);
                DelEpollIO(m_events[i].data.fd,0);
                continue;
            }
            
            if (m_events[i].data.fd == listenfd){
                /*accept*/
                
            }
            else{
                /*handle read or write*/
            }
            
        }
#endif
    }
}

void EventSetCallback()
{
    
}

