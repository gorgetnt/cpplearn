
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
//#include<thread>
#include<arpa/inet.h>
#include<error.h>
#include<sys/time.h>
#include<stdio.h>
#include<stdlib.h>
//实现一个tcp客户的连接断
#define BUFFER_WRITE "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmnopqrstuvwxyz\r\n\0"
struct ConnectContext
{

};
int connect_server(char * addr,int port)
{
    int serverFd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=inet_addr(addr);
    serverAddr.sin_port=htons(port);
    if(connect(serverFd,(struct sockaddr*)&serverAddr,sizeof(struct sockaddr_in))<0)
    perror("connect");
    return serverFd;
}
void send_recv_tcp(int fd,char * wBuffer,int wLen)
{
    ssize_t wres=send(fd,wBuffer,wLen,0);
    if(wres<=0)
    {
        perror("send");
    }
    char rBuffer[128];
    ssize_t rRes=recv(fd,rBuffer,128,0);
    if(rRes<=0)
    {
        perror("recv");
    }
}
int main(int argc,char * argv[])
{
    // 打印 argc 和 argv，确认参数传递

    int opt;
    while(opt=getopt(argc,argv,"s:p:t:c:n")!=-1)
    {
        printf("%d\n",opt);
        switch(opt)
        {
            case 's':
            printf("-s:%s",optarg);
            break;
            case 'p':
            printf("-s:%s",optarg);
            break;
            case 't':
            printf("-s:%s",optarg);
            break;
            case 'c':
            printf("-s:%s",optarg);
            break;
            case 'n':
            printf("-s:%s",optarg);
            break;
            case '?':
            break;
        }
    }
    return 0;
}