#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

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
    struct pollfd myfd[1024]; // poll可以无限多个，他不是和位图有关的
    for(int i=0;i<1024;i++)
    {
        myfd[i].fd=-1;
        myfd[i].events=POLLIN;
    }
    myfd[0].fd=sockfd;
    
    int maxfd=0; // 表示myfd已经使用了的最大的下标

    printf("Server started, waiting for connections...\n");
    
    
    
    char buf[1024];
    while(1)
    {
        int ret=poll(myfd,maxfd+1,-1); // 阻塞检查，有事件发生才会返回，执行失败返回-1
        if(ret<0)
        {
            perror("poll failed");
            close(sockfd);
            return -1;
        }
        if(myfd[0].revents&POLLIN) // 监听sockfd的可读事件
        {
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
                for(int i=1;i<=1024;i++)
                {
                    if(myfd[i].fd==-1)
                    {
                        myfd[i].fd=cfd;
                        myfd[i].events=POLLIN;
                        maxfd=i>maxfd?i:maxfd;
                        break;
                    }
                }
                
            }
        }
        // 有通讯的时候
        // 其实本质上类似轮询，但是不一样的是只有当读缓冲区有数据的时候才会触发通讯
        for(int i=1;i<=maxfd;i++)
        {
            if(myfd[i].fd!=-1&&myfd[i].revents&POLLIN) // 如果有输入事件
            {
                ret=recv(myfd[i].fd,buf,sizeof(buf),0);
                if(ret<0)
                {
                    perror("recv faild");// 出错应该有错误处理，这里只是简单的打印错误信息
                    continue;
                }
                if(ret==0)
                {
                    printf("client disconnect\n");
                    close(myfd[i].fd); // 关闭描述符
                    myfd[i].fd=-1;
                    myfd[i].events=POLLIN;
                    continue;
                }
                if(ret>0)
                {
                    buf[ret]='\0';
                    printf("client say:%s",buf);
                    send(myfd[i].fd,buf,strlen(buf),0);
                    memset(buf,0,sizeof(buf));
                }
            }
        }
    }
    close(sockfd);
    return 0;
}

