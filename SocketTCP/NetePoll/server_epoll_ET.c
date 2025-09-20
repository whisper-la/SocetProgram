#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

/*
    - 该程序设置为边缘触发模式，即来一次新数据仅仅触发一次，而非缓冲区有事件的情况下就一直触发
    - 边缘触发需要保证能够一次性把缓冲区的数据全部读走，否则数据只能等到下次触发才能够被处理
    方法一：设置一个足够大缓冲区，缺点浪费内存资源
    方法二：循环读取数据，直到读取到数据的长度小于缓冲区的大小，说明数据已经全部读取完毕。
            但是由于read和write是阻塞的，所以需要用fcntl设置O_NONBLOCK，非阻塞IO，当读取数据时如果数据长度小于缓冲区的大小，说明数据已经全部读取完毕；
            但是在非阻塞模式夏，若没有数据或者缓冲区满，使用read和write可能会导致读取错误，所以需要函数调用失败的时候，如果errno为EAGAIN,就不再perror。
*/


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

    // 端口复用
    int opt=1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

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

    int epfd=epoll_create(1024);
    if(epfd<0)
    {
        perror("epoll_create failed");
        close(sockfd);
        return -1;
    }

    // 添加sockfd到epoll实例中
    struct epoll_event ev;  
    ev.data.fd=sockfd;
    ev.events=EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&ev); // 添加sockfd到epoll实例中

    struct epoll_event evs[100];
    int size=sizeof(evs)/sizeof(evs[0]);
    printf("Server started, waiting for connections...\n");
    
    while(1)
    {
        // evs是传出的参数，包含了事件 ，num是事件的数量，-1表示阻塞等待，超时时间为-1表示永久阻塞等待，size是evs的大小
        int num=epoll_wait(epfd,evs,size,-1); // 成功的话返回事件的数量
        if(num<0)
        {
            perror("epoll_wait failed");
            close(sockfd);
            return -1;
        }
        else if(num==0)
        {
            printf("epoll_wait timeout\n");
            continue;
        }

        for(int i=0;i<num;i++) // 对于每一个事件
        {
            if(evs[i].data.fd==sockfd && (evs[i].events&EPOLLIN)) // 如果是sockfd的事件
            {
                // 连接到客户端
                struct sockaddr_in client_addr;
                socklen_t client_addr_len=sizeof(client_addr);
                int cfd=accept(sockfd,(struct sockaddr*)&client_addr,&client_addr_len);
                if(cfd<0)
                {
                    perror("accept failed");
                    close(sockfd);
                    return -1;
                }
                else {// 连接成功
                    printf("client connected, ip:%s,port:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
                    // 设置文件描述符为非阻塞
                    int flag = fcntl(cfd, F_GETFL);
                    flag |= O_NONBLOCK;                                                        
                    fcntl(cfd, F_SETFL, flag);

                    // 添加通讯的文件描述符到epoll实例中
                    ev.data.fd=cfd;
                    ev.events=EPOLLIN|EPOLLET; // 边沿触发
                    if(epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev)!=0) // 添加到监听失败
                    {
                        perror("epoll_ctl failed");
                        close(cfd);
                        continue;
                    }
                }
            }
            else if(evs[i].data.fd!=sockfd && (evs[i].events&EPOLLIN)) // 如果是客户端的事件
            {
                char buf[5];
                memset(buf,0,sizeof(buf));
                while (1)
                {
                    // 客户端的事件
                    int ret=recv(evs[i].data.fd,buf,sizeof(buf),0);
                    if(ret<0)
                    {
                        if(errno==EAGAIN)
                        {
                            break; // 非阻塞IO，数据已读完，是正常情况，直接跳出循环
                        }
                        else if(errno==ECONNRESET) {
                            // 对端连接重置，意味着连接已异常断开，必须清理
                            printf("Client reset the connection.\n");
                            epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, NULL);
                            close(evs[i].data.fd);
                            break;
                        }
                        else{
                            // 其他未知错误，也建议直接关闭连接，因为连接很可能也不可用了
                            perror("recv faild");
                            epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, NULL);
                            close(evs[i].data.fd);
                            break;
                        }
                    }
                    else if(ret==0)
                    {
                        printf("client disconnect\n");
                        // 从epoll实例中移除
                        epoll_ctl(epfd,EPOLL_CTL_DEL,evs[i].data.fd,NULL);// 先删除epoll实例中的描述符，后关闭，否则会删除失败，持续监测
                        close(evs[i].data.fd); // 关闭描述符
                        break;
                    }
                    else // 收到数据
                    {
                        printf("client say:%s\n",buf);
                        send(evs[i].data.fd,buf,strlen(buf),0);
                    }
                }
            }
        }
    }
    close(sockfd);
    return 0;
}

