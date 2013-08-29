#include "epoller.h"

CEpoller::CEpoller()
    :_epfd(-1)
    ,_epoll_size(0)
    ,_max_events(0)
    ,_events(NULL)
{
}

CEpoller::~CEpoller()
{
    destroy();
    delete []_events;
    _events = NULL;
}

void CEpoller::create(int epoll_size)
{
    _epoll_size = epoll_size;
    _max_events = epoll_size;
    
    _epfd = epoll_create(_epoll_size);
    if (_epfd == -1){
        /*failed*/
    }
}

void CEpoller::destroy()
{
    if (_epfd != -1){
        close(_epfd);
        _epfd = -1;
    }
}

