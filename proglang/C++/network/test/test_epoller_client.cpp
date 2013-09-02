#include "socket.h"

int main(int argc, char *argv[])
{
    CSock client_sock;
    
    struct sockaddr_in servaddr;
    int addrlen = sizeof(servaddr);
    char msg[]="hello,world!";
    char recvline[1024];
    
    char sIP[]="192.168.107.208";
    int  nPort = 5000;
    
    client_sock.Create();
    client_sock.Connect(sIP,nPort);
    

#if 0    
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
        send(sockfd,msg,strlen(msg),0);
        int recvlen = recv(sockfd,recvline,1024,0);
        recvline[recvlen] = '\0';
        printf("received:%s\n",recvline);
    }
    
    close(sockfd);
#endif    
    return 0;
}
