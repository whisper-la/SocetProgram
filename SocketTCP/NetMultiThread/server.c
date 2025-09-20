#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
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
    if(listen(sockfd,128)<0)
    {
        perror("listen failed");
        close(sockfd);
        return -1;
    }
    
    printf("Server started, waiting for connections...\n");
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_len=sizeof(client_addr);
    int cfd=accept(sockfd,(struct sockaddr*)&client_addr,&client_addr_len);
    if(cfd<0)
    {
        perror("accept failed");
        close(sockfd);
        return -1;
    }else 
    {
        printf("client ip:%s,port:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
    }
    
    char buf[1024];
    while(1)
    {
        int ret=recv(cfd,buf,sizeof(buf),0);
        if(ret<0)
        {
            perror("recv faild");
            break;
        }
        if(ret==0)
        {
            printf("client disconnect\n");
            break;
        }
        if(ret>0)
        {
            buf[ret]='\0';
            printf("client say:%s",buf);
            send(cfd,buf,strlen(buf),0);
            memset(buf,0,sizeof(buf));
        }
    }
    
    close(sockfd);
    close(cfd);

    return 0;
}

