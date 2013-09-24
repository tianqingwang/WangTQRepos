#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include  <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CONNECTION   4 
#define NPORT            5000

int allsockets[MAX_CONNECTION];

static int setnonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}

int main(int argc, char *argv[])
{
    int i;
    int sockfd;
    int reuse_on = 1;
    char sendmsg[] = "Hello,Server!";
    char sendmsg1[] = "This is the second message!";
    char recvmsg[1024];
    
    for (i=0; i<MAX_CONNECTION; i++){
        sockfd = socket(AF_INET,SOCK_STREAM,0);
        if (sockfd < 0){
            perror("Failed to create socket.");
            return -1;
        }
        
        setnonblock(sockfd);
        setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse_on,sizeof(reuse_on));
        
        allsockets[i] = sockfd;
        
        struct sockaddr_in serv_addr;
        bzero(&serv_addr,sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(NPORT);
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    }
    
    for (i=0; i<MAX_CONNECTION;i++){
        int sendlen = send(allsockets[i],sendmsg,strlen(sendmsg),0);
        if (sendlen < 0){
            fprintf(stderr,"fd=%d sent error\n",allsockets[i]);
        }
        else if (strlen(sendmsg) == sendlen){
            fprintf(stdout,"fd=%d sent the msg: %s\n",allsockets[i],sendmsg);
        }
        else{
            fprintf(stderr,"fd=%d not sent all msg.\n",allsockets[i]);
        }
    }
    
    usleep(100000);

    for (i=0; i<MAX_CONNECTION;i++){
        int recvlen = recv(allsockets[i],recvmsg,1024,0);
        if (recvlen <= 0){
            fprintf(stderr,"fd=%d recv error\n",allsockets[i]);
        }
        else{
            recvmsg[recvlen] = '\0';
            fprintf(stdout,"fd=%d receive the msg:%s\n",allsockets[i],recvmsg);
            memset(recvmsg,0,1024);
        }
    }    
    
#if 0    
    usleep(100000);
    
    for (i=0; i<MAX_CONNECTION;i++){
        int sendlen = send(allsockets[i],sendmsg1,strlen(sendmsg1),0);
        if (sendlen < 0){
            fprintf(stderr,"fd=%d sent error\n",allsockets[i]);
        }
        else if (strlen(sendmsg1) == sendlen){
            fprintf(stdout,"fd=%d sent the msg: %s\n",allsockets[i],sendmsg1);
        }
        else{
            fprintf(stderr,"fd=%d not sent all msg.\n",allsockets[i]);
        }
    }
#endif   
    
    while(1){
        sleep(1);
    }    
    return 0;
}
