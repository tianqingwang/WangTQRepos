#ifndef __LIBEVENT_EVENT_H__
#define __LIBEVENT_EVENT_H__

typedef struct user_data_s{
    int            fd;
    //int            dstfd;
    int            datalen;
    char          *pdata;
    struct timeval accept_time;
}user_data_t;


void set_max_connection(int nMaxConnection);
void signal_process();
void main_loop(int sockfd);
void main_loop_exit();
#endif
