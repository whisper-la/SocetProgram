#include <iostream>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "tcpSever.hpp"
#include "tcpSocket.hpp"

struct Socket_info
{
    TcpSocket *tcp;
    TcpSever *s;
    struct sockaddr_in client_addr;
};

struct Socket_info infos[128];

void* thread_fun(void* arg)
{
    struct Socket_info* pinfo=static_cast<struct Socket_info*>(arg);
    std::cout << "client ip:" << inet_ntoa(pinfo->client_addr.sin_addr) << ",port:" << ntohs(pinfo->client_addr.sin_port) << std::endl;
    std::string buf;
    while (1)
    {    
        int ret=pinfo->tcp->recvData(buf);
        if(ret<0)
        {
            std::cout << "recv failed"<< std::endl;
            break;
        }
        if(ret==0)
        {
            std::cout << "client ip:" << inet_ntoa(pinfo->client_addr.sin_addr) << ",port:" << ntohs(pinfo->client_addr.sin_port) << " disconnect" << std::endl;
            break;
        }
        if(ret>0)
        {
            std::cout << buf << std::endl;
            pinfo->tcp->sendData(buf);
            buf.clear();
        }
    }
    delete pinfo->tcp;
    pinfo->tcp=NULL;
    delete pinfo;
    pinfo=NULL;
    return NULL;
}

int main(int argc, char const *argv[])
{
    TcpSever sever;
    int ret=sever.setListen(8888);
    if(ret<0)
    {
        std::cout << "setListen failed" << std::endl;
        return -1;
    }
    std::cout << "Server started, waiting for connections..." << std::endl;

    while (true)
    {
        Socket_info *Socketinfo = new Socket_info;
        TcpSocket* tcp=sever.acceptClient(Socketinfo->client_addr);
        if(ret<0)
        {
            std::cout << "acceptClient failed" << std::endl;
            delete tcp;
            return -1;
        }
        std::cout << "acceptClient success" << std::endl; 

        pthread_t tid;
        Socketinfo->tcp=tcp;
        Socketinfo->s=&sever;
        pthread_create(&tid,NULL,thread_fun,(void*)Socketinfo);
        pthread_detach(tid);
    }

    return 0;
}
