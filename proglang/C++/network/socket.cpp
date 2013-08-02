#include "socket.h"




CSock::Listen(int backlog){
    return listen(m_sockfd,backlog);
}