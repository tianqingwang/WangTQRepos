#ifndef __C_EPOLLER_H__
#define __C_EPOLLER_H__

#include <sys/epoll.h>

#define MAXEVENTS   (1000)


class CEpoller
{
public:
    CEpoller();
    ~CEpoller();
    
    void create(int epoll_size);
    void destroy();
    int timed_wait(int milliseconds);
    
    /*events*/
    get_events();
    set_events();
    del_events();

private:
    int _epfd;
    int _epoll_size;
    int _max_events;
    struct epoll_event *_events;
};

#endif
