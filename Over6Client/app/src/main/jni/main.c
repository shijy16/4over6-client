#include "over6_over6client_MainActivity.h"
#include "util.h"
const int MAX_BUFFER = 4105;

//管道相关
int ip_handle;
int data_handle;
char* ip_name = "/data/data/over6.over6client/ip";
char* data_name = "/data/data/over6.over6client/data";
char ip_buffer[4105];
char data_buffer[4105];
char sock_buffer[4105];

int write_handle(int handle,char* msg){
    char b[1024] = "";
    bzero(b, sizeof(b));
    sprintf(b, "%s\0",msg);
    return write(handle, b, strlen(b) + 1);
}

int read_handle(char* name,char* read_buffer){
    int handle = open(name, O_RDWR|O_CREAT);
    memset(read_buffer, 0, MAX_BUFFER*sizeof(char));
    int len = read(handle, read_buffer, MAX_BUFFER);
    close(handle);
    return len;
}

void connect_to_server(){
    //创建socket连接
    int client_socket;
    if((client_socket = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
        write_handle(ip_handle,"socket创建失败\n");
        LOG("socket创建失败!\n");
    }

    struct sockaddr_in6 server_socket;
    bzero(&server_socket, sizeof(server_socket));
    server_socket.sin6_family = AF_INET6;
    server_socket.sin6_port = htons(SERVER_PORT);
    inet_pton(AF_INET6, SERVER_ADDR, &server_socket.sin6_addr);

    LOG("尝试连接到服务器\n");
    if(connect(client_socket, (struct sockaddr *) &server_socket, sizeof(server_socket)) == 0) {
        write_handle(ip_handle,"成功连接服务器\n");
        LOG("成功连接服务器\n");
    } else {
        write_handle(ip_handle,"连接服务器失败\n");
        LOG("连接服务器失败\n");
    }
    struct Msg msg;
    bzero(&msg, sizeof(msg));
    msg.length = sizeof(msg);
    msg.type = IP_REQ;
    memcpy(sock_buffer, &msg, sizeof(struct  Msg));
    if(send(client_socket, sock_buffer, sizeof(struct Msg), 0) <= 0){
        LOG("发送ip请求失败");
    }

    int notClosed = 1;
    while(notClosed) {
        // Now Receive Package
        bzero(sock_buffer, MAX_BUFFER);

        int len = recv(client_socket, sock_buffer, 4, 0);
        if(len == -1 || len == 0) {
            LOG("接收出错，连接断开");
            break;
        }
        int sz = *(int*) sock_buffer - 4;
        int i = 0;
        for(i = 0; i < sz; ++i) {
            recv(client_socket, sock_buffer+4+i, 1, 0);
        }
        LOG("Receive %d Bytes From Server!\n", len+sz);

    }

    close(client_socket);
}

int main(void){
    LOG("C starting");
    //初始化管道
    mknod(ip_name, 0666, 0);//创建ip管道
    ip_handle = open(ip_name, O_RDWR|O_CREAT|O_TRUNC);
    mknod(data_name, 0666, 0);//创建流量管道
    data_handle = open(data_name, O_RDWR|O_CREAT|O_TRUNC);
    connect_to_server();

    sleep(3);
    if(read_handle(data_name,data_buffer) > 0){
        LOG("succeed to receive data");
    }else{
        LOG("failed to receive data");
    }
    //连接到服务器

    //关闭管道
    close(ip_handle);
    close(data_handle);
}

JNIEXPORT jstring JNICALL Java_over6_over6client_MainActivity_StringFromJNI
        (JNIEnv *env, jobject thisz){
    main();
    return (*env)->NewStringUTF(env,"hello from JNI");
}