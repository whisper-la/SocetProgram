#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

typedef struct fdinfo
{
    int fd; // 文件描述符
    fd_set *readfds; // 要被操作的文件描述符集
    int *maxfd; // 最大的文件描述符
}fdinfo_t;


// 创建用于文件描述符共享的互斥锁
pthread_mutex_t mutex;

void* connect_to_client(void* arg)
{
    fdinfo_t *pfdinfo=(fdinfo_t*)arg;
    printf("thread connect id:%ld\n",pthread_self());
    struct sockaddr_in client_addr;
    socklen_t client_addr_len=sizeof(client_addr);
    int cfd=accept(pfdinfo->fd,(struct sockaddr*)&client_addr,&client_addr_len);
    if(cfd<0)
    {
        perror("accept failed");
        // 不要关闭监听套接字，只需要释放资源
        free(pfdinfo); // 创建在堆上的内存要free掉
        pthread_exit(NULL);
    }else 
    {
        printf("client ip:%s,port:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        pthread_mutex_lock(&mutex);
        FD_SET(cfd,pfdinfo->readfds);
        if(cfd>(*pfdinfo->maxfd)) // 不断用cfd往里面扩容
        {
            *pfdinfo->maxfd=cfd;
        }
        pthread_mutex_unlock(&mutex);
        // 释放内存
        free(pfdinfo);
        pthread_exit(NULL);
    }
}


void* communication(void* arg)
{
    fdinfo_t *pfdinfo=(fdinfo_t*)arg;
    printf("thread communication id:%ld\n",pthread_self());
    char buf[1024];
    int ret=recv(pfdinfo->fd,buf,sizeof(buf)-1,0); // 预留一个字节给结束符
    if(ret<0)
    {
        if(ret<0) {
            if(errno == EBADF) {
                // 因为程序是并发的，所以可能会出现描述符已经被关闭的情况
                // 当上一个线程因为断开连接已经关闭了文件描述符，然而后续因为字符描述集是set的，又创建了线程，这就导致可能会出现recv failed: Bad file descriptor
                //所以避免一下此现象
                printf("client already disconnected\n"); 
            } else {
                perror("recv failed");
            }
            // 清理资源
        }
        free(pfdinfo); // 创建在堆上的内存要free掉
        pthread_exit(NULL);
    }
    if(ret==0)
    {
        printf("client disconnect\n");
        pthread_mutex_lock(&mutex);
        FD_CLR(pfdinfo->fd,pfdinfo->readfds); // 从读集合中移除
        pthread_mutex_unlock(&mutex);
        close(pfdinfo->fd); // 关闭描述符
        free(pfdinfo); // 创建在堆上的内存要free掉
        pthread_exit(NULL);
    }
    if(ret>0)
    {
        buf[ret]='\0';
        printf("client say:%s",buf);
        send(pfdinfo->fd,buf,ret,0); // 使用实际接收到的字节数
        free(pfdinfo); // 释放内存
        pthread_exit(NULL);
    }
    // 意外情况也释放资源
    free(pfdinfo);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0){
        perror("create socket failed");
        return -1;
    }
    
    // 设置套接字选项，允许地址重用
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
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

    pthread_mutex_init(&mutex,NULL);
    while(1)
    {
        // 得等描述符更新完了，线程执行完了再去select，不然新的文件描述符没加进来没有用了
        pthread_mutex_lock(&mutex);
        fdstmp=readfds;
        // 在锁的保护下读取maxfd
        int current_maxfd = maxfd;
        pthread_mutex_unlock(&mutex);
        
        // 设置合理的超时时间，避免程序完全阻塞, 可能的问题是导致创建了多个线程用于连接，其中部分线程则阻塞住，空占用系统资源
        // 可以将连接的线程放回到主线程，不用单独线程处理
        // 或者设置条件变量， 阻塞主线程更新描述符集，直到连接完毕，但是并不适用于多个同时连接
        struct timeval timeout={5,0}; // 5秒超时
        // 使用局部变量current_maxfd
        int ret=select(current_maxfd+1,&fdstmp,NULL,NULL,&timeout); 
        if(ret<0)
        {
            perror("select failed");
            close(sockfd);
            pthread_mutex_destroy(&mutex);
            return -1;
        }
        else if(ret==0)
        {
            // 超时处理，可以添加心跳或其他逻辑
            printf("select timeout, continuing...\n");
            continue;
        }

        // 监听到新的连接就开启线程接受连接
        if(FD_ISSET(sockfd,&fdstmp))  // 否则就是大于0
        {
            fdinfo_t *fdinfo=(fdinfo_t*)malloc(sizeof(fdinfo_t)); // 创建到堆上
            if (fdinfo == NULL) {
                perror("malloc failed");
                continue;
            }
            fdinfo->fd=sockfd;
            fdinfo->readfds=&readfds;
            fdinfo->maxfd=&maxfd;
            
            pthread_t tid;
            pthread_create(&tid,NULL,connect_to_client,fdinfo);
            pthread_detach(tid);
        }
        // 有通讯的时候
        // 其实本质上类似轮询，但是不一样的是只有当读缓冲区有数据的时候才会触发通讯
        for(int i=0;i<=current_maxfd;i++) // 修复：使用局部变量current_maxfd
        {
            if(i!=sockfd&&FD_ISSET(i,&fdstmp)) // 如果被置位了，并且该描述符不是sockfd，那么i就是通讯的描述符
            {
                fdinfo_t *fdinfo=(fdinfo_t*)malloc(sizeof(fdinfo_t)); // 创建到堆上
                if (fdinfo == NULL) {
                    perror("malloc failed");
                    continue;
                }
                fdinfo->fd=i;
                fdinfo->readfds=&readfds;
                fdinfo->maxfd=&maxfd;
                
                pthread_t tid;
                pthread_create(&tid,NULL,communication,fdinfo);
                pthread_detach(tid);
            }
        }
    }
    
    // 清理资源的代码（理论上不会执行到这里）
    close(sockfd);
    pthread_mutex_destroy(&mutex);
    return 0;
}

