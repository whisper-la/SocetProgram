#include <iostream>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "threadpool.hpp"
#include "tcpSever.hpp"
#include "tcpSocket.hpp"

struct Socket_info
{
    TcpSocket *tcp;
    struct sockaddr_in client_addr;
};


void thread_fun(void* arg)
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
}

void connect_client(void* arg)
{

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

    threadPool<Socket_info> pool(4,8); // 最小4个线程，最大8个线程

    int addrlen=sizeof(struct sockaddr_in);
    while(1)
    {
        struct Socket_info *infos = new Socket_info;
        infos->tcp=sever.acceptClient(infos->client_addr);
        if(infos->tcp==nullptr)
        {
            std::cout << "acceptClient failed" << std::endl;
            delete infos;
            continue;
        }
        std::cout << "acceptClient success" << std::endl; 
        pool.addTask(thread_fun,(void*)infos); // 向线程池中添加任务
    }

    return 0;
}
