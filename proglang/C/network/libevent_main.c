#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "libevent_socket.h"
#include "libevent_event.h"
#include "log.h"

#define  MAX_NOFILE  4096
#define  MAX_CONNECTION 4096

int main(int argc, char *argv[])
{
    int listen_fd;
    struct event *ev_accept;
    struct rlimit rt;
    
    initLogInfo();

#if 0    
    if (getrlimit(RLIMIT_NOFILE,&rt) == -1){
        printf("getrlimit\n");
    }
    
    printf("rlim_max=%d,rlim_cur=%d\n",rt.rlim_max,rt.rlim_cur);
    
    rt.rlim_max = rt.rlim_cur = MAX_NOFILE;
    
    
    if (setrlimit(RLIMIT_NOFILE,&rt) == -1){
        printf("setrlimit\n");
    }
#endif
    
    set_rlimit(RLIM_INFINITY,MAX_CONNECTION);
 
    set_max_connection(MAX_CONNECTION);
    fd_init(MAX_CONNECTION);
    
    signal_process();

    listen_fd = socket_setup(5000);
    
    if (listen_fd == -1){
        logInfo(LOG_ERR,"socket_setup failed.");
        exit(1);
    }

    main_loop(listen_fd);
}
