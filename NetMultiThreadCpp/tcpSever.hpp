#pragma once
#include "tcpSocket.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>


class TcpSever
{
public:
    TcpSever();
    ~TcpSever();
    int setListen(unsigned short port);
    TcpSocket* acceptClient(sockaddr_in &client);
private:
    int m_fd;
};

TcpSever::TcpSever()
{
    m_fd = socket(AF_INET, SOCK_STREAM, 0); // 创建socket通讯，用于服务器端
    if (m_fd == -1)
    {
        perror("socket");
    }
}

TcpSever::~TcpSever()
{
    if (m_fd != -1)
    {
        close(m_fd);
    }
}
int TcpSever::setListen(unsigned short port)
{
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    server_addr.sin_addr.s_addr=INADDR_ANY; // INADDR_ANY绑定本机任意IP，是0.0.0.0，一般用于本地的绑定操作
    if (bind(m_fd,reinterpret_cast<struct sockaddr *>(&server_addr),sizeof(server_addr))<0)     
    {
        perror("bind failed");
        return -1;
    }
    if (listen(m_fd,  128) < 0)
    {
        perror("listen");
        return -1;
    }
    return 0; // 成功监听
}

TcpSocket* TcpSever::acceptClient(sockaddr_in &client_addr)
{
    socklen_t client_addr_len = sizeof(client_addr);
    int fd = accept(m_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);
    if (fd == -1)
    {
        perror("accept");
        return nullptr;
    }
    return new TcpSocket(fd);
}
