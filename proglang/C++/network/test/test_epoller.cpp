#include "socket.h"
#include "epoller.h"

void AcceptCallback(int fd, int events, void *args);
void ReadCallback(int fd, int events, void *args);
void WriteCallback(int fd, int events, void *args);

int main(int argc, char *argv[])
{
    CSock serv_sock;
    CEpoller serv_epoll;
    
    char *sIP = NULL;
    int  nPort = 5000;
    
    if (!serv_sock.Create(sIP,nPort)){
        printf("can't create server socket\n");
        return -1;
    }
    
    if (!serv_sock.Listen()){
        printf("can't listen the server socket\n");
        return -1;
    }
    
    serv_epoll.Init(1000,500,10);
    
    serv_epoll.EventLoop(serv_sock.GetSocket());
}

int AcceptCallback(int fd, int events, void *args)
{
    int connfd;
    
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    
    connfd = accept(fd,(struct sockaddr*)&clientaddr,&len);
    
    if (connfd == -1){
        return -1;
    }
    
    return connfd;
}