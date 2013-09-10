#include <unistd.h>
#include "socket.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    CSock client_sock;
    char buf[]="hi,it's me!";
    struct sockaddr_in servaddr;
    int addrlen = sizeof(servaddr);
    char msg[]="hello,world!";
    char recvline[1024];
    
    char sIP[]="192.168.107.208";
    int  nPort = 5000;
    
    int i = 0;
//    for (i=0; i<1000; i++){
    client_sock.Create();
    client_sock.Connect(sIP,nPort);
    
    client_sock.Send(client_sock.GetSocket(),buf,strlen(buf));
    
    client_sock.Send(client_sock.GetSocket(),msg,strlen(msg));

//    }
//    client_sock.Send(client_sock.GetSocket(),buf,strlen(buf));
//    client_sock.Receive(client_sock.GetSocket(),recvline,1024);
//    printf("%s\n",recvline);
    client_sock.Close();
    return 0;
}
