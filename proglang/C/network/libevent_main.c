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

#define  MAX_CONNECTION 4096
#define  SOCKET_PORT    5000

int main(int argc, char *argv[])
{
    int listen_fd = -1;
    struct event *ev_accept;
    struct rlimit rt;
    
    initLogInfo();
    
    set_rlimit(RLIM_INFINITY,MAX_CONNECTION);
 
    set_max_connection(MAX_CONNECTION);
    fd_init(MAX_CONNECTION);
    
    signal_process();

    listen_fd = socket_setup(SOCKET_PORT);
    
    if (listen_fd == -1){
        logInfo(LOG_ERR,"socket_setup failed.");
        exit(1);
    }

    main_loop(listen_fd);
}
