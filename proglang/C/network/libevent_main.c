#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "libevent_socket.h"
#include "libevent_multi.h"

#define NPORT      5000
#define BACKLOG    5
#define MAXTHREADS 3

void read_callback(int fd);
void write_callback(int fd,char *buf);

int main(int argc, char *argv[])
{
    int sockfd = socket_setup(NPORT);
    
    pthread_t test_id; 

    if (sockfd == -1){
        fprintf(stderr,"socket_setup failed.\n");
        return 1;
    }
    /*set worker threads.*/
    
    worker_thread_init(MAXTHREADS);
    set_read_callback(read_callback);
    set_write_callback(write_callback);
    //pthread_create(&test_id,NULL,(void*)&test_func,NULL);
    master_thread_loop(sockfd);
    
    return 0;
}

static int count = 0;

void read_callback(int fd)
{
    char buf[1024];
    int n;
    
    n = read(fd,buf,1024);
    if (n > 0){
        fprintf(stdout,"(%d)%s: server received string:%s\n",count++,__FILE__,buf);
        memset(buf,0,1024);
    }
}

void write_callback(int fd, char *buf)
{
    
}

