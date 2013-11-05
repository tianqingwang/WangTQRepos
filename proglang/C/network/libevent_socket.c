#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include "libevent_socket.h"
#include "log.h"

#define TIMEOUT_SEC       (30)   /*300 seconds*/

static fd_t *g_fd;
pthread_mutex_t fd_lock = PTHREAD_MUTEX_INITIALIZER;

int set_socket_nonblock(int fd)
{
    int flag;
    flag = fcntl(fd,F_GETFL,NULL);
    if (flag < 0){
        logInfo(LOG_ERR,"fcntl GETFL error.");
        return -1;
    }
    
    flag |= O_NONBLOCK;
    
    if (fcntl(fd,F_SETFL,flag) < 0){
        logInfo(LOG_ERR,"fcntl SETFL error.");
        return -1;
    }
    
    return 0;
}

int set_socket_linger(int fd)
{
    struct linger linger;
    
    linger.l_onoff = 1;
    linger.l_linger = 0;
    
    return setsockopt(fd,SOL_SOCKET,SO_LINGER,&linger,sizeof(struct linger));
}

int set_socket_reusable(int fd)
{
    int reuse_on = 1;
    return setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&reuse_on,sizeof(reuse_on));
}

int socket_setup(int nPort)
{
    int listenfd;
    struct sockaddr_in listen_addr;
    int reuse = 1;
    
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd < 0){
        logInfo(LOG_ERR,"failed to create listening socket.");
        return -1;
    }
    
    /*set socket reuseable*/
    if (set_socket_reusable(listenfd) < 0){
        logInfo(LOG_ERR,"failed to set listening socket re-usable.");
        return -1;
    }
    
    /*set socket non-blocking*/
    if (set_socket_nonblock(listenfd) < 0){
        logInfo(LOG_ERR,"failed to set listening socket non-blocking.");
        return -1;
    }
    
//    int nrecvbuf = 1024*1024;
//    setsockopt(listenfd,SOL_SOCKET,SO_RCVBUF,(const char*)&nrecvbuf,sizeof(int));
    
    memset(&listen_addr,0,sizeof(struct sockaddr_in));
    listen_addr.sin_family      = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port        = htons(nPort);
    if (bind(listenfd,(struct sockaddr*)&listen_addr,sizeof(listen_addr)) < 0){
        logInfo(LOG_ERR,"failed to bind the listening socket.");
        return -1;
    }
    
    if (listen(listenfd,BACKLOG) < 0){
        logInfo(LOG_ERR,"failed to listen the socket.");
        return -1;
    }
    
    return listenfd;
}

int set_rlimit(long nMaxCore,int nMaxConnection)
{
    struct rlimit rlim;
    getrlimit(RLIMIT_CORE,&rlim);
    rlim.rlim_max = rlim.rlim_cur = (rlim_t)nMaxCore;
    if (setrlimit(RLIMIT_CORE,&rlim) == -1){
        logInfo(LOG_ERR,"set core limit failed.");
        return -1;
    }
    
    getrlimit(RLIMIT_NOFILE,&rlim);
    rlim.rlim_max = rlim.rlim_cur = (rlim_t)nMaxConnection;
    if (setrlimit(RLIMIT_NOFILE,&rlim) == -1){
        logInfo(LOG_ERR,"set file limit failed.");
        return -1;
    }
    
    return 0;
}

/*check socket fd close_wait timeout*/
static int g_max_connection = 0;

void fd_init(int nMaxConnection)
{
    int i;
    
    g_fd = (fd_t*)malloc(sizeof(fd_t)*nMaxConnection);
    
    g_max_connection = nMaxConnection;
    
    for (i=0; i<nMaxConnection; i++){
        g_fd[i].fd = -1;
        g_fd[i].last_time = 0;
    }
}

int fd_insert(int fd)
{
    if (fd < 0){
        logInfo(LOG_WARN,"fd < 0");
        return -1;
    }
    
    if (fd >= g_max_connection-1){
        logInfo(LOG_WARN,"fd > MAX_CONNECTIONS-1");
        return -1;
    }
    
    pthread_mutex_lock(&fd_lock);
    
    g_fd[fd].fd = fd;
    g_fd[fd].last_time = time(NULL);
    
    logInfo(LOG_INFO,"fd=%d is inserted.",fd);
    
    pthread_mutex_unlock(&fd_lock);
    
    return 0;
}

int fd_del(int fd)
{
    if (fd < 0){
        return -1;
    }
    
    close(fd);
    pthread_mutex_lock(&fd_lock);
    g_fd[fd].fd = -1;
    g_fd[fd].last_time = 0;
    pthread_mutex_unlock(&fd_lock);
    return 0;
}

int fd_update_last_time(int fd,time_t last_time)
{
    int ret = -1;
    
    pthread_mutex_lock(&fd_lock);
    
    if (g_fd[fd].fd != -1){
        g_fd[fd].last_time = last_time;
        ret = 0;
    }
    
    pthread_mutex_unlock(&fd_lock);
    
    return ret;
}

void fd_free(){
    free(g_fd);
}

int check_closewait_timeout(time_t now)
{
    int i;
    
    for (i=0; i<g_max_connection; i++){
        if (g_fd[i].fd != -1){
            logInfo(LOG_INFO,"i=%d,g_fd[%d].fd=%d,last_time=%u",i,i,g_fd[i].fd,g_fd[i].last_time);
            if (now - g_fd[i].last_time >= TIMEOUT_SEC){
                logInfo(LOG_INFO,"fd=%d will be closed.",g_fd[i].fd);
                fd_del(g_fd[i].fd);
            }
        }
    }

}
