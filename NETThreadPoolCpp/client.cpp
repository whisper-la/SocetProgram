#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "tcpSocket.hpp"

int main()
{
    int number=0;
    TcpSocket tcp;
    int ret=tcp.connectToHost("127.0.0.1",8888);
    if(ret<0)
    {
        return -1;
    }

    // int fd1=open("1.txt",O_RDONLY);
    // if(fd1<0)
    // {
    //     perror("open faild");
    //     return -1;
    // }


    while(1)
    {
        // std::string str="hello server";
        // str.resize(32);
        // int ret1=read(fd1,&str[0],sizeof(str));
        // if(ret1<0)
        // {
        //     perror("read faild");
        //     return -1;
        // }
        std::string str = std::string("The answer is ") + std::to_string(number++);
        // str[0]=static_cast<char>(number++);
        ret=tcp.sendData(str);
        if(ret<0)
        {
            std::cout<<"send faild"<<std::endl;
            return -1;
        }
        str.clear();
        ret=tcp.recvData(str);
        if(ret<0)
        {
            std::cout<<"recv faild"<<std::endl;
            return -1;
        }
        else if(ret==0)
        {
            std::cout<<"server close"<<std::endl;
            break;
        }
        std::cout<<"server say:"<<str<<std::endl;
        str.clear();
        sleep(1);
    }
    // close(fd1);
    return 0;
}
