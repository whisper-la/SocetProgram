#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0)
    {
        perror("create socket faild");
        return -1;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(8888);
    inet_pton(AF_INET, "192.168.116.130", &server_addr.sin_addr.s_addr);

    printf("connect server\n");
    int number=0;
    char buf[1024];
    while(1)
    {
        sprintf(buf,"hello, i want to say%d\n",number++);
        int ret=sendto(sockfd,buf,strlen(buf)+1,0,(struct sockaddr*)&server_addr,sizeof(server_addr));
        if(ret<0)
        {
            perror("send faild");
            return -1;
        }
        memset(buf,0,sizeof(buf));

        ret=recvfrom(sockfd,buf,sizeof(buf),0,NULL ,NULL); // 不太需要主机的ip地址，故设为NULL
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
