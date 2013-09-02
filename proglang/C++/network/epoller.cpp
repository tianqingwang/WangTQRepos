#include <string.h>
#include <fcntl.h>
#include "epoller.h"


char buf[1024];

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
                    AddEpollIO(retval,EPOLLOUT|EPOLLET);
                }
            }
            else{
                /*handle read or write*/
                if (HandleReadWrite(m_events[i].data.fd,m_events[i].events) < 0){
                    continue;
                }
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


