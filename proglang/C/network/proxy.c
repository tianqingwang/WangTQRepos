#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <event.h>

#define LISTEN_PORT    5000
#define CONNECT_PORT   5001

static struct event_base *base;

static const char listen_addr[] = "192.168.107.208";
static const char connect_addr[] = "192.168.107.209";
static int connect_addrlen;

static void eventcb(struct bufferevent *bev, short which, void *ctx);

static void readcb(struct bufferevent *bev, void *ctx)
{
    struct bufferevent *partner = ctx;
    struct evbuffer *src, *dst;
    
    size_t len;
    
    src = bufferevent_get_input(bev);
    len = evbuffer_get_length(src);
    
    if (!partner){
        evbuffer_drain(src,len);
        return;
    }
    
    dst = bufferevent_get_output(partner);
    evbuffer_add_buffer(dst,src);
}

static void close_on_finished_writecb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *b = bufferevent_get_output(bev);
    if(evbuffer_get_length(b) == 0){
        bufferevent_free(bev);
    }
}

static void eventcb(struct bufferevent *bev, short which, void *ctx)
{
    struct bufferevent *partner = ctx;
    
    if (which & (BEV_EVENT_EOF | BEV_EVENT_ERROR)){
        /*log*/
        if (partner){
            /*Flush all pending data*/
            readcb(bev,ctx);
            
            if (evbuffer_get_length(bufferevent_get_output(partner))){
                bufferevent_setcb(partner,NULL,close_on_finished_writecb,eventcb,NULL);
                bufferevent_disable(partner,EV_READ);
            }
            else{
                bufferevent_free(partner);
            }
        }
        
        bufferevent_free(bev);
    }
} 

int main(int argc, char *argv[])
{
    
}