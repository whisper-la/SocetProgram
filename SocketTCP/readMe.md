# 网络编程TCP
- NetMultiThread: 单线程/多线程实现网络编程  
gcc -o multiThreadSever multiThreadSever.c 
gcc -o client client.c 
gcc -o server_Test server.c
- NetThreadPool: 线程池实现网络编程  
g++ -o Poolserver ThreadpoolSever.c threadpool.c
gcc -o client client.c 


- NetMultiThreadCpp: 单线程/多线程实现网络编程(使用C++实现)
g++ -o multiThreadSever multiThreadSever.cpp  
g++ -o client client.cpp

- NetThreadPoolCpp: 线程池实现网络编程(使用C++实现)
g++ -o Poolserver Poolserver.cpp
g++ -o client client.cpp

- NetSelect: 选择器实现网络编程
gcc -o server server.c
gcc -o client client.c

- NetSelectMultiThread: 选择器实现网络编程(多线程)
gcc -o server server.c
gcc -o client client.c
但是这个程序还有线程竞态问题，理论上不应该是一个IO多路复用创建多个线程。
应该参考主从 Reactor 多线程，如果有时间再去研究
**例如 可以这样写**
线程1循环select监听套接字描述符，当读缓冲区有数据时，就用条件变量通知线程2（线程2的生产者）
线程2循环accept，当被唤醒后就accept，然后进行连接，连接完毕后选择工作线程并加入到其select的文件描述符中（线程1的消费者）

工作线程负责接收和通讯


- NetPoll: 轮询器实现网络编程
gcc -o server server.c
gcc -o client client.c