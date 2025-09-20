#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "threadpool.h"

struct client_info
{
    int cfd;
    struct sockaddr_in client_addr;
};
typedef struct
{
    threadpool_t* pool;
    int fd;
}PoolInfo;



void thread_fun(void* arg)
{
    struct client_info* pinfo=(struct client_info*)arg;
    printf("client ip:%s,port:%d\n",inet_ntoa(pinfo->client_addr.sin_addr),ntohs(pinfo->client_addr.sin_port));
    char buf[1024];
    while (1)
    {    
        int ret=recv(pinfo->cfd,buf,sizeof(buf),0);
        if(ret<0)
        {
            perror("recv failed");
            break;
        }
        if(ret==0)
        {
            printf("client ip:%s,port:%d disconnect\n",inet_ntoa(pinfo->client_addr.sin_addr),ntohs(pinfo->client_addr.sin_port));
            break;
        }
        if(ret>0)
        {
            buf[ret]='\0';
            printf("client say:%s",buf);
            send(pinfo->cfd,buf,strlen(buf),0);
            memset(buf,0,sizeof(buf));
        }
    }
    close(pinfo->cfd);
}

void connect_client(void* arg)
{
    PoolInfo* infos=(PoolInfo*)arg;
    socklen_t addrlen=sizeof(struct sockaddr_in);
    while(1)
    {
        struct client_info *pinfo=(struct client_info *)malloc(sizeof(struct client_info));
        pinfo->cfd=accept(infos->fd,(struct sockaddr*)&pinfo->client_addr,&addrlen);
        if(pinfo->cfd<0)
        {
            perror("accept failed");
            close(infos->fd);
        }

        threadpool_add_task(infos->pool,thread_fun,(void*) pinfo); // 向线程池中添加任务
    }
}

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

    threadpool_t* pool=threadpool_create(3,12,100);

    PoolInfo *info=(PoolInfo *)malloc(sizeof(PoolInfo));
    info->pool=pool;
    info->fd=sockfd;
    threadpool_add_task(pool,connect_client,(void*)info); // 向线程池中添加任务
    pthread_exit(NULL); // 这种退出线程的方式能够保证子线程的存活
    return 0;
}