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
    
    
//    serv_epoll.EventSetCallback(&loop,read_event,EPOLL_READ_FLAG);
//    serv_epoll.EventSetCallback(&loop,write_event,EPOLL_WRITE_FLAG);

//    connection_t *c = serv_epoll.GetConnection(&loop,serv_sock.GetSocket());
    connection_t *c = new connection_t;
    c->fd = serv_sock.GetSocket();
    c->data = NULL;
    c->read = NULL;
    c->write = NULL;
    printf("server connection c=%p\n",c);
    serv_epoll.SetNonBlock(c->fd);
    printf("c->fd=%d,serv_sock->fd=%d\n",c->fd,serv_sock.GetSocket());
    
    if (c == NULL){
        printf("can't add listening socket.");
        return -1;
    }
    serv_epoll.ConnectionAdd(c);
    serv_epoll.SetListenSocketFD(serv_sock.GetSocket());
    
    serv_epoll.EventLoop(&loop);
    
#endif
}
