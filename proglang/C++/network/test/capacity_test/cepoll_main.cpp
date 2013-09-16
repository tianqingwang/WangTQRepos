#include "cepollclient.h"  
  
int main(int argc, char *argv[])  
{  
        CEpollClient *pCEpollClient = new CEpollClient(60000, "127.0.0.1", 5000);  
        if(NULL == pCEpollClient)  
        {  
                cout<<"[epollclient error]:main init"<<"Init CEpollClient fail"<<endl;  
        }  
  
        pCEpollClient->RunFun();  
  
        if(NULL != pCEpollClient)  
        {  
                delete pCEpollClient;  
                pCEpollClient = NULL;  
        }  
  
        return 0;  
} 
