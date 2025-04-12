#include<iostream>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<thread>
#include<arpa/inet.h>
#include<error.h>
#include<sys/time.h>
#include<stdio.h>
#include<vector>
using namespace std;
//实现一个tcp客户的连接断
#define BUFFER_WRITE "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789abcdefghijklmnopqrstuvwxyz\r\n\0"
#define TIMEVAL_DIFF_US(start, end) \
    (((end).tv_sec - (start).tv_sec) * 1000000LL + \
     ((end).tv_usec - (start).tv_usec))

struct ConnectContext
{
    char * serverAddr;
    int serverPort;
    int threadNum;
    int requestion;
    int connection;
}ptx;
int connect_server(char * addr,int port)
{
    int serverFd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=inet_addr(addr);
    serverAddr.sin_port=htons(port);
    if(connect(serverFd,(struct sockaddr*)&serverAddr,sizeof(struct sockaddr_in))<0)
    perror("connect");
    //cout<<"connect:"<<addr<<":"<<port<<endl;
    return serverFd;
}
void send_recv_tcp(int fd,char * wBuffer,int wLen)
{
    ssize_t wres=send(fd,wBuffer,wLen,0);
    if(wres<0)
    {
        perror("send");
    }
    // else
    // {
    //     //cout<<"send->"<<wBuffer<<"  count:"<<wres<<endl;
    // }
    // char rBuffer[128];
    // ssize_t rRes=recv(fd,rBuffer,128,0);
    // if(rRes<=0)
    // {
    //     perror("recv");

    // }
}
void test_qps_entry(ConnectContext &&ptx)
{
    int count=ptx.requestion/ptx.threadNum;
    #if 0 //百万并发的实现
    int countCon=ptx.connection/ptx.threadNum;
    auto serverFds=make_unique<int[]>(countCon*2);
    for(int i=0;i<ptx.countCon;i++)
    {
        int fd=connect_server(ptx.serverAddr,ptx.serverPort);
        serverFds[fd]=fd;
    }
    #endif
    int fd=connect_server(ptx.serverAddr,ptx.serverPort);
    for(int i=0;i<count;i++)
    {
        send_recv_tcp(fd,BUFFER_WRITE,sizeof(BUFFER_WRITE));
    }
}
int main(int argc,char * argv[])
{
    struct timeval timeBegin;
    gettimeofday(&timeBegin,NULL);
    int opt;
    while((opt=getopt(argc,argv,"s:p:t:c:n:?"))!=-1)
    {
        switch(opt)
        {
            case 's':
            printf("-s:%s\n",optarg);
            ptx.serverAddr=optarg;
            break;
            case 'p':
            printf("-p:%s\n",optarg);
            ptx.serverPort=atoi(optarg);
            break;
            case 't':
            printf("-t:%s\n",optarg);
            ptx.threadNum=atoi(optarg);
            break;
            case 'c':
            printf("-c:%s\n",optarg);
            ptx.connection=atoi(optarg);
            break;
            case 'n':
            printf("-n:%s\n",optarg);
            ptx.requestion=atoi(optarg);
            break;
            case '?':
            cout<<"输入有误"<<endl;
            return -1;
            break;
        }
    }
    //解析完毕
    vector<thread> threadPool;
    for(int i=0;i<ptx.threadNum;i++)
    {
        threadPool.emplace_back(thread(test_qps_entry,ptx));
    }
    for(thread & t:threadPool)
    {
        t.join();
    }
    struct timeval timeEnd;
    gettimeofday(&timeEnd,NULL);
    long long dif=TIMEVAL_DIFF_US(timeBegin,timeEnd);
    cout<<"timeused:"<<dif<<endl;
    return 0;
}