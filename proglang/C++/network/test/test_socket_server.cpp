#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "socket.h"

#define NET_PORT    2046

int main(int argc, char *argv[])
{
    char buf[1024] = {0};
    char sendbuf[]="server reply: Hey,Jude!\n";
    char sIP[] ="192.168.107.208";
    
    /*create a server*/
    CSock serv_sock;
    
    
    int clientfd;
    struct sockaddr_in clientaddr;
    int socklen = sizeof(clientaddr);
    
    
    if (!serv_sock.Create(sIP,NET_PORT)){
        printf("create server socket error\n");
        return -1;
    }
    
    if (!serv_sock.Listen()){
        printf("error to listen\n");
        return -1;
    }
    
    /*accept*/
    while(1){
        clientfd = serv_sock.Accept((struct sockaddr*)&clientaddr,&socklen);
        if (clientfd < 0){
            fprintf(stderr,"failed to setup connection with client\n");
            continue;
        }
        
        if (fork() == 0){
            /*child process*/
            serv_sock.Close();
            int recvlen = serv_sock.Receive(clientfd,buf,1024);
            printf("recvlen=%d\n",recvlen);
            printf("received msg:%s\n",buf);
            
            /*send msg*/
            serv_sock.Send(clientfd,sendbuf,strlen(sendbuf));
            exit(0);
        }
        else{
            /*parent process*/
            close(clientfd);
        }
    }
    
}