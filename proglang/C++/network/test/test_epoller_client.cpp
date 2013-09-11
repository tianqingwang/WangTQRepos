#include <unistd.h>
#include <fcntl.h>
#include "socket.h"
#include <string.h>
#include <stdio.h>

#define BUFSIZE    (4096)

int main(int argc, char *argv[])
{
    CSock client_sock;
    char buf[]="hi,it's me!";
    struct sockaddr_in servaddr;
    int addrlen = sizeof(servaddr);
    char msg[]="hello,world!";
    char recvline[BUFSIZE];
    
    char sIP[]="192.168.1.105";
    int  nPort = 5000;
    
    char filename[] = "black.bmp";    
 
    int i = 0;
//    for (i=0; i<10; i++){
    client_sock.Create();
    client_sock.Connect(sIP,nPort);
    
    int len = 0;
    int fileid = open(filename,O_RDONLY);

    while(1){
        len = read(fileid,recvline,BUFSIZE);
        if (len < 0){
            break;
        }
        if (len >= 0 && len < BUFSIZE){
            client_sock.Send(client_sock.GetSocket(),recvline,len);
            break;
        }
        if (len == BUFSIZE){
            client_sock.Send(client_sock.GetSocket(),recvline,BUFSIZE);
        }

         
    }
//    client_sock.Send(client_sock.GetSocket(),buf,strlen(buf));
    
//    client_sock.Send(client_sock.GetSocket(),msg,strlen(msg));

//    }
//    client_sock.Send(client_sock.GetSocket(),buf,strlen(buf));
//    client_sock.Receive(client_sock.GetSocket(),recvline,1024);
//    printf("%s\n",recvline);
    client_sock.Close();
    return 0;
}
