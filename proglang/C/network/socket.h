#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/types.h>
#include <sys/socket.h>
#incude <netdb.h>

class CSock{
public:
    CSock(){ /*construct function*/
        m_sockfd = -1;
    }
    ~CSock(); /*deconstruct function*/
    bool Create(); /*setup socket*/
    bool SetSockOpt();
    bool Bind();
    bool Listen(int backlog = 5);
    bool Connect();
    bool Accept();
    int  Send();
    int  Receive();
	
    bool CloseSocket();
private:
	int m_sockfd;
};

#endif
