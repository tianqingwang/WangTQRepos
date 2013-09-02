#include "socket.h"
#include "epoller.h"

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
    serv_epoll.AttachSocket(&serv_sock);
    
    serv_epoll.EventLoop();
}
