#ifndef _DEFINE_EPOLLCLIENT_H_  
#define _DEFINE_EPOLLCLIENT_H_  
#define _MAX_SOCKFD_COUNT 65535  
  
#include<iostream>  
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <fcntl.h>  
#include <arpa/inet.h>  
#include <errno.h>  
#include <sys/ioctl.h>  
#include <sys/time.h>  
#include <string.h>  
  
using namespace std;  
  
/** 
 * @brief �û�״̬ 
 */  
typedef enum _EPOLL_USER_STATUS_EM  
{  
        FREE = 0,  
        CONNECT_OK = 1,//���ӳɹ�  
        SEND_OK = 2,//���ͳɹ�  
        RECV_OK = 3,//���ճɹ�  
}EPOLL_USER_STATUS_EM;  
  
/*@brief 
 *@CEpollClient class �û�״̬�ṹ�� 
 */  
struct UserStatus  
{  
        EPOLL_USER_STATUS_EM iUserStatus;//�û�״̬  
        int iSockFd;//�û�״̬������socketfd  
        char cSendbuff[1024];//���͵���������  
        int iBuffLen;//�����������ݵĳ���  
        unsigned int uEpollEvents;//Epoll events  
};  
  
class CEpollClient  
{  
        public:  
  
                /** 
                 * @brief 
                 * ������:CEpollClient 
                 * ����:���캯�� 
                 * @param [in] iUserCount  
                 * @param [in] pIP IP��ַ 
                 * @param [in] iPort �˿ں� 
                 * @return �޷��� 
                 */  
                CEpollClient(int iUserCount, const char* pIP, int iPort);  
  
/** 
                 * @brief 
                 * ������:CEpollClient 
                 * ����:�������� 
                 * @return �޷��� 
                 */  
                ~CEpollClient();  
  
                /** 
                 * @brief 
                 * ������:RunFun 
                 * ����:�����ṩ�Ľӿڣ�����epoll�� 
                 * @return �޷���ֵ 
                 */  
                int RunFun();  
  
        private:  
  
                /** 
                 * @brief 
                 * ������:ConnectToServer 
                 * ����:���ӵ������� 
                 * @param [in] iUserId �û�ID 
                 * @param [in] pServerIp ���ӵķ�����IP 
                 * @param [in] uServerPort ���ӵķ������˿ں� 
                 * @return �ɹ�����socketfd,ʧ�ܷ��ص�socketfdΪ-1 
                 */  
                int ConnectToServer(int iUserId,const char *pServerIp,unsigned short uServerPort);  
  
/** 
                 * @brief 
                 * ������:SendToServerData 
                 * ����:�������������û�(iUserId)������ 
                 * @param [in] iUserId �û�ID 
                 * @return �ɹ����ط������ݳ��� 
                 */  
                int SendToServerData(int iUserId);  
  
                /** 
                 * @brief 
                 * ������:RecvFromServer 
                 * ����:�����û��ظ���Ϣ 
                 * @param [in] iUserId �û�ID 
                 * @param [in] pRecvBuff ���յ��������� 
                 * @param [in] iBuffLen ���յ����ݳ��� 
                 * @return �ɹ����ؽ��յ����ݳ��ȣ�ʧ�ܷ��س���Ϊ-1 
                 */  
                int RecvFromServer(int iUserid,char *pRecvBuff,int iBuffLen);  
  
                /** 
                 * @brief 
                 * ������:CloseUser 
                 * ����:�ر��û� 
                 * @param [in] iUserId �û�ID 
                 * @return �ɹ�����true 
                 */  
                bool CloseUser(int iUserId);  
  
/** 
                 * @brief 
                 * ������:DelEpoll 
                 * ����:ɾ��epoll�¼� 
                 * @param [in] iSockFd socket FD 
                 * @return �ɹ�����true 
                 */  
                bool DelEpoll(int iSockFd);  
        private:  
  
                int    m_iUserCount;//�û�������  
                struct UserStatus *m_pAllUserStatus;//�û�״̬����  
                int    m_iEpollFd;//��Ҫ����epollfd  
                int    m_iSockFd_UserId[_MAX_SOCKFD_COUNT];//���û�ID��socketid��������  
                int    m_iPort;//�˿ں�  
                char   m_ip[100];//IP��ַ  
};  
  
#endif  