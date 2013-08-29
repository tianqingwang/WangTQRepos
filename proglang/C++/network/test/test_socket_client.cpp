#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socket.h"

#define NET_PORT 2046

int main(int argc,char *argv[])
{
    CSock client_sock;
    struct sockaddr_in servaddr;
    int nlen = sizeof(servaddr);
    
    char sendbuf[]="Hello,world!\n";
    char recvbuf[1024]={0};
    
    char sIP[] = "192.168.107.208";
    
    if (!client_sock.Create()){
        printf("can't create socket\n");
        return -1;
    }
#if 0    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(NET_PORT);
    inet_pton(AF_INET,sIP,&servaddr.sin_addr);
    
    if (client_sock.Connect((const struct sockaddr*)&servaddr,nlen) < 0){
        perror("connect failed");
        return -1;
    }
#else
    if (client_sock.Connect(sIP,NET_PORT) < 0){
        perror("connect failed");
        return -1;
    }
#endif
    
    client_sock.Send(client_sock.GetSocket(),sendbuf,strlen(sendbuf));
    client_sock.Receive(client_sock.GetSocket(),recvbuf,1024);
    printf("%s\n",recvbuf);
    client_sock.Close();
}