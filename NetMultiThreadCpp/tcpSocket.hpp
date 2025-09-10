#pragma once
#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

class TcpSocket
{
public:
    TcpSocket();
    TcpSocket(int fd);
    ~TcpSocket();
    int connectToHost(const std::string &ip, unsigned short port);
    int sendData(const std::string &data);
    int recvData(std::string &data);

private:
    int m_fd;
    int writen(const char *buf, int size);
    int readn(char *buf, int size);
};

TcpSocket::TcpSocket()
{
    m_fd = socket(AF_INET, SOCK_STREAM, 0); // 创建socket通讯，用于客户端
    if (m_fd == -1)
    {
        perror("socket");
    }
}

TcpSocket::TcpSocket(int fd)
{
    m_fd = fd; // 服务器端为accept返回的fd
}

TcpSocket::~TcpSocket()
{
    if (m_fd != -1)
    {
        close(m_fd);
    }
}

int TcpSocket::connectToHost(const std::string &ip, unsigned short port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str()); // 点分十进制转换为网络字节序
    int ret = connect(m_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("connect");
    }
    return ret;
}


//  返回值为发送长度，如果发送错误则为-1
int TcpSocket::sendData(const std::string &data)
{
    char* p = static_cast<char*>(new char[data.size() + 4]); // 获得足够大内存
    // 带长度发送
    if(data.size() == 0)
    {
        return -1;
    }
    int len = htonl(data.size());

    memcpy(p, &len, 4);
    memcpy(p + 4, data.c_str(), data.size());
    int ret = writen(p, data.size()+4);
    if (ret == -1)
    {
        perror("writen");
    }
    delete[] p;
    return ret;
}


//  返回值为接收长度，如果接受错误则为-1
int TcpSocket::recvData(std::string &data)
{
    
    // 先接收长度
    int len = 0;
    int ret = readn(reinterpret_cast<char*>(&len), 4);
    if (ret == -1)
    {
        perror("readn");
        return -1;
    }
    else if(ret==0)
    {
        return 0;
    }
    len=ntohl(len);
    // 接收数据
    data.resize(len);
    ret = readn(&data[0], len);
    if (ret == -1)
    {
        perror("readn");
        return -1;
    }
    else if(ret!=len)
    {
        std::cout << "length is not matched" << std::endl;
        return -1;
    }
    return ret;
}

int TcpSocket::writen(const char *buf, int size)
{
    int count=0;
    while(count<size)
    {
        int ret=write(m_fd,buf+count,size-count);
        if(ret==-1)
        {
            return -1;
        }
        else if(ret==0)
        {
            continue;
        }
        else
        {
            count+=ret;
        }
    }
    return count;
}

int TcpSocket::readn(char *buf, int size)
{
    int count=0;
    while(count<size)
    {
        int ret=read(m_fd,buf+count,size-count);
        if(ret==-1)
        {
            return -1;
        }
        else if(ret==0)
        {
            return 0;
        }
        else
        {
            count+=ret;
        }
    }
    return count;
}
