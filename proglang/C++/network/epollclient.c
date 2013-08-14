#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#define MSGLEN   (2048)

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in servaddr;
    int addrlen = sizeof(servaddr);
    char msg[2048]={0};
    char recvline[1024];
    
    char sIP[]="127.0.0.1";
    int  nPort = 58888;
    
    int fd = open("afhan",O_RDONLY);
    if (fd < 0){
        perror("open error");
        return -1;
    }
    
    sockfd = socket(AF_INET,SOCK_STREAM,0);
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
    else{
        int n = read(fd,msg,MSGLEN);
        send(sockfd,msg,n,0);
        printf("client sent:%s\n\n",msg);
        int recvlen = recv(sockfd,recvline,MSGLEN,0);
        recvline[recvlen] = '\0';
        printf("received:%s\n",recvline);
    }
    
    close(sockfd);
    
    return 0;
}
