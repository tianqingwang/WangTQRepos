#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define MSGLEN   (2048)
#if 1
void *thread_func1(int *_sockfd)
{   
    int readlen = 0;
    int fd = open("afhan",O_RDONLY);
    char msg[1024];
    msg[0] = 0;
    
    int sockfd = *_sockfd;
    
    printf("thread_func1 sockfd=%d\n",sockfd);
    
    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);
    
    
    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);
    
    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);
    
    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);

    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);

    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);
    
    close(fd);
    
}

void *thread_func2(int *_sockfd){
    int readlen = 0;
    int fd = open("aiji",O_RDONLY);
    char msg[1024];
    msg[0] = 0;
    
    int sockfd = *_sockfd;
    
    printf("thread_func2 sockfd=%d\n",sockfd);
    
    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);
    
    
    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);
    
    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);
    
    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);

    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);

    readlen = read(fd,msg,1024);
    if (readlen > 0){
        send(sockfd,msg,readlen,0);
    }
    msg[0] = 0;
//    sleep(1);
    
    close(fd);
}



#endif

int main(int argc, char *argv[])
{
    int sockfd,sockfd1;
    int fd;
    int readlen;
    struct sockaddr_in servaddr;
    int addrlen = sizeof(servaddr);
    char msg[2048]={0};
    char recvline[1024];
    
    char sIP[]="127.0.0.1";
    int  nPort = 58888;
#if 0    
    fd = open("afhan",O_RDONLY);
    if (fd < 0){
        perror("open error");
        return -1;
    }
#endif    
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    printf("sockfd = %d\n",sockfd);
    if (sockfd == -1){
        perror("socket error.");
        return -1;
    }
    
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(sIP);
    servaddr.sin_port = htons(nPort);
    
    if (connect(sockfd,(struct sockaddr*)&servaddr,addrlen) < 0){
        perror("connect failed");
        return -1;
    }
    
    sockfd1 = socket(AF_INET,SOCK_STREAM,0);
    printf("sockfd = %d\n",sockfd);
    if (sockfd == -1){
        perror("socket error.");
        return -1;
    }
    if (connect(sockfd1,(struct sockaddr*)&servaddr,addrlen) < 0){
        perror("connect failed");
        return -1;
    }
    
    pthread_t thread_id1;
    pthread_t thread_id2;
//    pthread_t thread_id3;
    
    pthread_create(&thread_id1,NULL,thread_func1,&sockfd);
    pthread_create(&thread_id2,NULL,thread_func2,&sockfd1);
    
//    pthread_create(&thread_id3,NULL,thread_recv1,&sockfd);
    
//    while(recv(sockfd,recvline,1024,0)>0){
//        printf("%s",recvline);
//        recvline[0]=0;
//    }  
    
        
    
    pthread_join(thread_id1,NULL);
    pthread_join(thread_id2,NULL);
    
    close(sockfd);
    close(sockfd1);

    return 0;
}
