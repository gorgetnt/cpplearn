#include<sys/select.h>
#include<poll.h>
#include<sys/epoll.h>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<stdio.h>
#include<error.h>
#include<cstring>
#include<string>
#include<string.h>
#define _GNU_SOURCE
using namespace std;
#define BUFFER_LENGTH 1024
typedef enum RESPONSE_TYPE
{JPEG,HTML}responseTYPE;
struct conn_item
{
    char rBuffer[BUFFER_LENGTH];
    int rLen=0;
    char wBuffer[BUFFER_LENGTH];
    int wLen=0;
    struct epoll_event ev;
};
struct conn_item connList[1024];
int epfd;
void set_event(conn_item & conItem,uint32_t setEvent)
{
    conItem.ev.events=setEvent;
    epoll_ctl(epfd,EPOLL_CTL_MOD,conItem.ev.data.fd,&conItem.ev);
}
//create a new connection
//param:server conn_item
int accept_con(conn_item & conItem)
{
    struct sockaddr_in clientAddr;
    socklen_t len=sizeof(clientAddr);
    int clientfd=accept(conItem.ev.data.fd,(struct sockaddr*)&clientAddr,&len);
    cout<<"new client:"<<clientfd<<endl;
    connList[clientfd].ev.events=EPOLLIN;
    connList[clientfd].ev.data.fd=clientfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,connList[clientfd].ev.data.fd,&connList[clientfd].ev);
    return clientfd;
}
int recv_con(conn_item & conItem)
{
    int count=recv(conItem.ev.data.fd,&conItem.rBuffer[conItem.rLen],BUFFER_LENGTH-conItem.rLen,0);
    if(count==0)
    {
        epoll_ctl(epfd,EPOLL_CTL_DEL,conItem.ev.data.fd,&conItem.ev);
        close(conItem.ev.data.fd);
        cout<<"disconnect"<<conItem.ev.data.fd<<endl;
    }
    //cout<<"clientfd:"<<conItem.ev.data.fd<<"\t"<<"recv->"<<conItem.rBuffer<<endl;
    conItem.rLen+=count;
    return count;
}
//everytime send all data of wBuffer
//解析HTTP头，然后提取后缀，然后根据后缀
bool parse_http_url(char * httpRequest,char * url,size_t urlSize)
{
    if(httpRequest==nullptr) return false;
    char * begin=strchr(httpRequest,' ');
    
    if(begin==nullptr) return false;
    char * end=strchr(begin+1,' ');
    // int count=end-begin;
    // cout<<count<<endl;
    // for(int i=0;i<count;i++)
    // {
    //     cout<<begin[i]<<endl;
    // }
    memset(url,0,urlSize);
    size_t leng=end-begin;
    if(leng>=urlSize) return false;
    memcpy(url,begin+1,leng-1);
    url[leng]='\0';
    // cout<<url<<endl;
    return true;
}
int send_con(conn_item & conItem)
{
    set_event(conItem,EPOLLOUT);
    int count=send(conItem.ev.data.fd,conItem.wBuffer,conItem.wLen,0);
    memset(conItem.wBuffer,0,conItem.wLen);
    cout<<"clientfd:"<<conItem.ev.data.fd<<"\t"<<"send->"<<conItem.wBuffer<<"\tcount:"<<conItem.wLen<<endl;
    set_event(conItem,EPOLLIN);
}
// 生成 HTTP 响应
void http_response(conn_item &conItem,RESPONSE_TYPE response,char * url) {
    char filename[256]="./source";
    char * httpResponseHead;
    strncat(filename, url, sizeof(filename) - strlen(filename) - 1);
    if (response == HTML)
    {
        httpResponseHead = "HTTP/1.1 200 OK\r\n";
        cout<<"filename:"<<filename<<endl;
        FILE *fp = fopen(filename, "r");
        memset(conItem.wBuffer, 0, BUFFER_LENGTH);
        conItem.wLen = 0;
        if (fp == NULL)
        {
            perror("open file");
            conItem.wLen = snprintf(conItem.wBuffer, BUFFER_LENGTH, "HTTP/1.1 404 Not Found\r\n\r\n");
            send_con(conItem);
        }
        else
        {
            // 发送 HTTP 头部
            conItem.wLen = snprintf(conItem.wBuffer, BUFFER_LENGTH,
                                    "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html; charset=UTF-8\r\n"
                                    "Transfer-Encoding: chunked\r\n"
                                    "Connection: keep-alive\r\n"
                                    "\r\n");
            send_con(conItem);
    
            // 分块发送文件内容
            char buffer[1024];
            size_t bytes_read;
            char chunk_header[16];
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
            {
                // 构造并发送分块头部（十六进制长度）
                int chunk_len = snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", bytes_read);
                conItem.wLen = chunk_len;
                memcpy(conItem.wBuffer, chunk_header, chunk_len);
                send_con(conItem);
    
                // 发送文件内容
                conItem.wLen = bytes_read;
                memcpy(conItem.wBuffer, buffer, bytes_read);
                send_con(conItem);
    
                // 发送分块尾
                conItem.wLen = 2;
                memcpy(conItem.wBuffer, "\r\n", 2);
                send_con(conItem);
            }
    
            // 发送结束块
            conItem.wLen = 5;
            memcpy(conItem.wBuffer, "0\r\n\r\n", 5);
            send_con(conItem);
            fclose(fp);
        }
    }
    else if(response == JPEG)
    {
        FILE *fp = fopen(filename, "rb"); // 二进制模式
        if (fp == NULL)
        {
            conItem.wLen = snprintf(conItem.wBuffer, BUFFER_LENGTH, "HTTP/1.1 404 Not Found\r\n\r\n");
            send_con(conItem);
        }
        else
        {
            conItem.wLen = snprintf(conItem.wBuffer, BUFFER_LENGTH,
                                    "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: image/png\r\n"
                                    "Transfer-Encoding: chunked\r\n"
                                    "Connection: keep-alive\r\n"
                                    "\r\n");
            send_con(conItem);
            // 分块发送逻辑与 HTML 相同
            char buffer[1024];
            size_t bytes_read;
            char chunk_header[16];
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
            {
                int chunk_len = snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", bytes_read);
                conItem.wLen = chunk_len;
                memcpy(conItem.wBuffer, chunk_header, chunk_len);
                send_con(conItem);
                conItem.wLen = bytes_read;
                memcpy(conItem.wBuffer, buffer, bytes_read);
                send_con(conItem);
                conItem.wLen = 2;
                memcpy(conItem.wBuffer, "\r\n", 2);
                send_con(conItem);
            }
            conItem.wLen = 5;
            memcpy(conItem.wBuffer, "0\r\n\r\n", 5);
            send_con(conItem);
            fclose(fp);
        }
    }
    
    
}

int init_server(int port)
{
    int serverFd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(port);
    serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(serverFd,(struct sockaddr*)&serverAddr,sizeof(struct sockaddr)))
    {
        perror("bind");
        return -1;
    }
    listen(serverFd,10);
    connList[serverFd].ev.data.fd=serverFd;
    connList[serverFd].ev.events=EPOLLIN;
    epfd=epoll_create(1);
    if(epfd<0)
    {perror("epoll_create");}
    epoll_ctl(epfd,EPOLL_CTL_ADD,connList[serverFd].ev.data.fd,&connList[serverFd].ev);
    return connList[serverFd].ev.data.fd;
}
int main(int argc,const char*argv[])
{
    int argport=(argc>1)?atoi(argv[1]):2048;
    cout<<argport<<endl;
    int serverfd=init_server(argport);
    struct epoll_event events[1024];
    while(1)
    {
        int n=epoll_wait(epfd,events,1024,-1);
        for(int i=0;i<n;i++)
        {
            if(events[i].events==EPOLLIN)
            {
                if(events[i].data.fd==serverfd)
                {
                    accept_con(connList[serverfd]);
                }
                else
                {
                    int count=recv_con(connList[events[i].data.fd]);
                    if (count > 0) 
                    {
                        //解析http的url
                        char url[128];
                        cout<<connList[events[i].data.fd].rBuffer<<endl;
                        int ret=parse_http_url(connList[events[i].data.fd].rBuffer,url,128);
                        if(!ret) cout<<"parse 失败"<<endl;
                        cout<<url<<endl;
                        //解析后缀
                        char * begin=strchr(url,'.');
                        char * end=strchr(begin,'\0');
                        int suffixLen=end-begin;
                        char * suffix=begin;
                        if(strcmp(suffix,".html")==0)
                        {

                            http_response(connList[events[i].data.fd], HTML, url);
                        }
                        else if(strcmp(suffix,".jpg")==0)
                        {
                            http_response(connList[events[i].data.fd], JPEG, url);
                        }
                        
                        
                    } 
                    
                }
            }
        }
    }
    return 0;
}