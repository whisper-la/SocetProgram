#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    int sockfd=socket(AF_INET,SOCK_DGRAM,0); // UDP报文
    if(sockfd<0){
        perror("create socket failed");
        return -1;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(8888);
    server_addr.sin_addr.s_addr=INADDR_ANY; // INADDR_ANY绑定本机任意IP，是0.0.0.0，一般用于本地的绑定操作
    if (bind(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("bind failed");
        close(sockfd);
        return -1;
    }
    
    printf("Server started, waiting for connections...\n");
    
    struct sockaddr_in client_addr;

    socklen_t client_addr_len=sizeof(client_addr);
    
    char buf[1024];
    while(1)
    {
        int ret= recvfrom(sockfd, buf, sizeof(buf), 0,(struct sockaddr*)&client_addr, &client_addr_len); // UDP接收数据
        if(ret<0)
        {
            perror("recv faild");
            break;
        }
        if(ret==0)
        {
            printf("client ip:%s,port:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
            printf("client disconnect\n\n");
            break;
        }
        if(ret>0)
        {
            printf("client ip:%s,port:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
            printf("client say:%s\n",buf);
            sendto(sockfd,buf,strlen(buf),0,(struct sockaddr*)&client_addr,client_addr_len); // UDP发送数据
            memset(buf,0,sizeof(buf));
        }
    }
    close(sockfd);

    return 0;
}

