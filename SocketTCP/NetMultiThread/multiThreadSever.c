#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

struct client_info
{
    int cfd;
    pthread_t tid;
    struct sockaddr_in client_addr;
};

struct client_info infos[128];

void* thread_fun(void* arg)
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
    pinfo->cfd=-1;
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

    

    int max=sizeof(infos)/sizeof(infos[0]);
    for(int i=0;i<max;i++)
    {
        memset(&infos[i],0,sizeof(infos[i]));
        infos[i].cfd=-1;
        infos[i].tid=-1;
    }

    socklen_t client_addr_len=sizeof(infos[0].client_addr);
    while(1)
    {
        // 找到未被使用的结构体
        struct client_info* pinfo=NULL;
        for(int i=0;i<max;i++)
        {
            if(infos[i].cfd==-1)
            {
                pinfo=&infos[i];
                break;
            }
            if(i == max-1)
            {
                sleep(1);
                i--;
            }
        }
        pinfo->cfd=accept(sockfd,(struct sockaddr*)&pinfo->client_addr,&client_addr_len);
        if(pinfo->cfd<0)
        {
            perror("accept failed");
            close(sockfd);
            return -1;
        }
        pthread_create(&pinfo->tid,NULL,thread_fun,(void*)pinfo);
        pthread_detach(pinfo->tid);
    }

    close(sockfd);
    return 0;
}
