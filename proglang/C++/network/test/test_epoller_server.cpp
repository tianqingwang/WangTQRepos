#include "socket.h"
#include "epoller.h"

void read_event(event_t *ev);
void write_event(event_t *ev);

void read_event(event_t *ev)
{
    printf("read event\n");
}

void write_event(event_t *ev)
{
    printf("write event\n");
}

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

#if 0    
    serv_epoll.Init(1000,500,10);
    serv_epoll.AttachSocket(&serv_sock);
    
    serv_epoll.EventLoop();
#else
    epoller_t  loop;
    
    loop.connection_n = 2000;
    loop.event_size   = 500;
    
    
    if (serv_epoll.EpollInit(&loop) == -1){
        printf("epoll_init failed\n");
    }
    
   
//    serv_epoll.SetNonBlock();
    
    
    serv_epoll.EventSetCallback(&loop,read_event,EPOLL_READ_FLAG);
    serv_epoll.EventSetCallback(&loop,write_event,EPOLL_WRITE_FLAG);
    
    connection_t c;
    c.fd = serv_sock.GetSocket();
    c.read = NULL;
    c.write = NULL;
    
    serv_epoll.ConnectionAdd(&c);
//    serv_epoll.ConnectionAdd(&c);
    serv_epoll.EventLoop(&loop);
    
#endif
}
