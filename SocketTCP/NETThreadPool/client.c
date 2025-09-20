#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
    {
        perror("create socket faild");
        return -1;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(8888);
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    if(connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("connect faild");
        return -1;
    }
    int number=0;
    char buf[1024];
    while(1)
    {
        sprintf(buf,"server hello, i want to say%d\n",number++);
        int ret=send(sockfd,buf,strlen(buf),0);
        if(ret<0)
        {
            perror("send faild");
            return -1;
        }
        memset(buf,0,sizeof(buf));

        ret=recv(sockfd,buf,sizeof(buf),0);
        if(ret<0)
        {
            perror("recv faild");
            return -1;
            break;
        }
        if(ret==0)
        {
            printf("server disconnect\n");
            break;
        }        
        if(ret>0)
        {
            buf[ret]='\0';
            printf("server say:%s\n",buf);
        }
        memset(buf,0,sizeof(buf));
        sleep(1);
    }
    close(sockfd);
    return 0;
}