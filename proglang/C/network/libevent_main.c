#include <unistd.h>
#include <stdio.h>
#include "libevent_socket.h"
#include "libevent_multi.h"

#define NPORT      5000
#define BACKLOG    5
#define MAXTHREADS 3


int main(int argc, char *argv[])
{
    int sockfd = socket_setup(NPORT);
    
    if (sockfd == -1){
        fprintf(stderr,"socket_setup failed.\n");
        return 1;
    }
    
    /*set worker threads.*/
    worker_thread_init(MAXTHREADS);
    
    master_thread_loop(sockfd);
    
    return 0;
}