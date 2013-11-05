#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "sock.h"
#include "data.h"
#include "client.h"

char sIP[] ="192.168.107.208";
int  nport = 5000;

void *start_process(void *arg)
{
    int type,state;
    int whichline;
    
    struct timeval start;
    struct timeval stop;
    int    writebytes;
    int    readbytes;
//    char   buffer[1024*1024]="test.\n";
    int i;
    
    CLIENT *this = arg;
#if 0
    for (i=0; i<this->lines->index; i++){
        printf("%s\n",this->lines->buf[i]);
    }
#endif

#if 1    
    gettimeofday(&start,NULL);
    int sockfd = new_socket(sIP,nport);
    gettimeofday(&stop,NULL);
    
    printf("sockfd = %d\n",sockfd);
    if (sockfd == -1){
        this->connection = 0;
        return;
    }
    else{
        this->connection     = 1;
        this->conn_elapsed   = (stop.tv_sec - start.tv_sec)*1000000 + (stop.tv_usec - start.tv_usec);
        printf("conn_elapsed:%ld\n",this->conn_elapsed);
    }
    
    /*send test*/
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,&type);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&state);
    
    int k = 0;
    
    gettimeofday(&start,NULL);
    for (i=0; i<this->repeats; i++){
        i = (((this->time > 0) && (this->repeats <= 0)) || (this->repeats == MAXREPS)) ? 0 : i;
        /*get one buffer,suppose buffer[k]*/
        whichline = k%this->lines->index;
        k++;
        //writebytes = socket_write(sockfd,buffer,strlen(buffer));
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type);
        writebytes = socket_write(sockfd,this->lines->buf[whichline],strlen(this->lines->buf[whichline]));
        this->nwrite_total ++;
        if (writebytes < 0){
            /*failed*/
            this->nwrite_fail ++;
        }
        else{
            this->writebytes += writebytes;
        }
        pthread_setcanceltype(type,NULL);
        pthread_testcancel();
    }
    gettimeofday(&stop,NULL);
    this->write_elapsed = (stop.tv_sec - start.tv_sec)*1000000 + (stop.tv_usec - start.tv_usec);
    
    socket_close(sockfd);
#endif
}
