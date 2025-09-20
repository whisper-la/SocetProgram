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
    fd_set readfds,fdstmp;
    FD_ZERO(&readfds);
    FD_SET(sockfd,&readfds);
    int maxfd=sockfd;
    printf("Server started, waiting for connections...\n");
    
    
    
    char buf[1024];
    while(1)
    {

        fdstmp=readfds;
        int ret=select(maxfd+1,&fdstmp,NULL,NULL,NULL); // 阻塞检查，有事件发生才会返回，执行失败返回-1
        if(ret<0)
        {
            perror("select failed");
            close(sockfd);
            return -1;
        }
        if(FD_ISSET(sockfd,&fdstmp))
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
                FD_SET(cfd,&readfds);
                if(cfd>maxfd) // 不断用cfd往里面扩容
                {
                    maxfd=cfd;
                }
            }
        }
        // 有通讯的时候
        // 其实本质上类似轮询，但是不一样的是只有当读缓冲区有数据的时候才会触发通讯
        for(int i=0;i<=maxfd;i++)
        {
            if(i!=sockfd&&FD_ISSET(i,&fdstmp)) // 如果被置位了，并且该描述符不是sockfd，那么i就是通讯的描述符
            {
                ret=recv(i,buf,sizeof(buf),0);
                if(ret<0)
                {
                    perror("recv faild");// 出错应该有错误处理，这里只是简单的打印错误信息
                    continue;
                }
                if(ret==0)
                {
                    printf("client disconnect\n");
                    FD_CLR(i,&readfds); // 从读集合中移除
                    close(i); // 关闭描述符
                    continue;
                }
                if(ret>0)
                {
                    buf[ret]='\0';
                    printf("client say:%s",buf);
                    send(i,buf,strlen(buf),0);
                    memset(buf,0,sizeof(buf));
                }
            }
        }
    }
    close(sockfd);
    return 0;
}

