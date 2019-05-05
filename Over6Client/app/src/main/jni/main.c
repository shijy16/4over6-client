#include "over6_over6client_MainActivity.h"
#include "util.h"

struct IP_recv{
    int is_recv;
    char my_ip[100];
    char route[100];
    char DNS[3][100];
} ip;

const int MAX_BUFFER = 4105;

//管道相关
int ip_handle;
int data_handle;

int tun_descrip;

pthread_mutex_t my_mutex;

char* ip_name = "/data/data/over6.over6client/ip";
char* data_name = "/data/data/over6.over6client/data";
char ip_buffer[4105];
char data_buffer[4105];
char sock_buffer[4105];
char in_buffer[4105];
char out_buffer[4105];

int client_socket;
int hb_time;

int not_closed = 1;
int in_time = 0;
int in_size = 0;
int out_time = 0;
int out_size = 0;



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

void timer(){
    int ticks = 20;
    while(not_closed){
        int cur_time = time((time_t*)NULL);
        if(cur_time - hb_time > 60){
            LOG("失去与服务器的连接，连接已断开\n");
            not_closed = 0;
            break;
        }
        read_handle(ip_name,ip_buffer);
        if(ip_buffer[0] == 'B' && ip_buffer[1] == 'y' && ip_buffer[2] == 'e'){
            LOG("用户终止后台线程\n");
            not_closed = 0;
            break;
        }

        ticks--;
        if(ticks == 0){
            struct Msg msg;
            bzero(&msg, sizeof(msg));
            msg.length = sizeof(msg);
            msg.type = HEARTBEAT;
            memcpy(sock_buffer, &msg, sizeof(struct  Msg));
            if(send(client_socket, sock_buffer, sizeof(struct Msg), 0) <= 0){
                LOG("发送heartbeat失败\n");
            }else{
                LOG("发送heartbeat成功\n");
            }
            ticks = 20;
        }
        //流量统计
        char temp[100];
        bzero(temp,100);
        sprintf(temp,"流量统计 %d %d %d %d",in_time,in_size,out_time,out_size);
        LOG("%s\n",temp);
        write_handle(data_handle,temp);
        sleep(1);

    }
}

void vpn(){
    LOG("vpn已启动%d\n",tun_descrip);
    bzero(in_buffer,MAX_BUFFER);
    fd_set fd_s;

    while(not_closed){
        FD_ZERO(&fd_s);
        FD_SET(tun_descrip ,&fd_s);
        if(select(tun_descrip + 1, &fd_s, NULL, NULL, NULL) > 0){
            struct Msg msg;
            bzero(&msg,sizeof(struct Msg));
            int length = read(tun_descrip, in_buffer, MAX_BUFFER);
            if(length == 0){
                continue;
            }else if(length == -1){
                LOG("TUN read ERROR\n");
                continue;
            }
            msg.length = 5 + length;
            msg.type = DATA_SEND;
            memcpy(msg.data, in_buffer, sizeof(struct Msg));
            bzero(in_buffer,MAX_BUFFER);
            memcpy(in_buffer, &msg, sizeof(struct Msg));
            if(send(client_socket, in_buffer, sizeof(struct Msg), 0) > 0){
                LOG("发送%d byte给服务器\n",msg.length);
            }
            bzero(in_buffer,MAX_BUFFER);

            //修改发送次数和长度
            pthread_mutex_lock(&my_mutex);
            in_time++;
            in_size += msg.length;
            pthread_mutex_unlock(&my_mutex);
        }
    }
}

void connect_to_server(){
    pthread_mutex_init(&my_mutex, NULL);

    //创建socket连接
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
        LOG("成功连接服务器\n");
    } else {
        write_handle(ip_handle,"ERROR\n");
        LOG("连接服务器失败\n");
        return;
    }
    //发送ip请求
    struct Msg msg;
    bzero(&msg, sizeof(msg));
    msg.length = sizeof(msg);
    msg.type = IP_REQ;
    memcpy(sock_buffer, &msg, sizeof(struct  Msg));
    if(send(client_socket, sock_buffer, sizeof(struct Msg), 0) <= 0){
        LOG("发送ip请求失败");
    }

    hb_time = time((time_t*)NULL);

    //创建timer线程
    pthread_t timer_thread;
    if(pthread_create(&timer_thread, NULL, timer, NULL) == -1){
        LOG("计时器线程创建失败!\n");
        return;
    }else{
        LOG("计时器线程创建成功!\n");
    }

    ip.is_recv = 0;
    while(not_closed) {
        //判断连接是否已经断开
        int cur_time = time((time_t*)NULL);
        if(cur_time - hb_time > 60){
            LOG("失去与主机的连接\n");
            not_closed = 0;
            break;
        }

        bzero(sock_buffer, MAX_BUFFER);

        int len = recv(client_socket, sock_buffer, 4, 0);
        if(len == -1 || len == 0) {
            continue;
        }
        int i = 0;
        for(i = 0; i < *(int*) sock_buffer - 4; i++) {
            recv(client_socket, sock_buffer+4+i, 1, 0);
        }
        bzero(&msg, sizeof(struct Msg));
        memcpy(&msg,sock_buffer,sizeof(struct Msg));
        LOG("接收到%d\n",msg.type);
        //服务器回复的IP请求
        if(msg.type == IP_REP){
            LOG("ip requst reply received\n");
            LOG("%s\n",msg.data);
            bzero(ip_buffer,sizeof(char)*MAX_BUFFER);
            //发送到前端
            write_handle(ip_handle,msg.data);
            sscanf(msg.data,"%s %s %s %s %s",&ip.my_ip,&ip.route,&ip.DNS[0],&ip.DNS[1],&ip.DNS[2]);
            ip.is_recv = 1;
            printf("%s\n",ip.my_ip);

            //从前端读取tun描述符
            sleep(1);
            memset(ip_buffer, 0, MAX_BUFFER);
            int len = read_handle(ip_name, ip_buffer);
            if(len <= 0) {
                write_handle(data_handle,"ERROR");
                LOG("读取tun描述符失败%s\n",ip_buffer);
                not_closed = 0;
                return;
            }
            //创建vpn线程
            tun_descrip = atoi(ip_buffer);
            LOG("读取tun描述符成功：%d\n",tun_descrip);
            pthread_t vpn_thread;
            pthread_create(&vpn_thread, NULL, vpn, NULL);

        }
        else if(msg.type == DATA_RECV){
            int sz;
            if (( sz = write(tun_descrip, msg.data, msg.length-5)) != msg.length - 5) {
                LOG("写入tun出错\n");
            }else{
                LOG("成功写入tun\n");
            }

            pthread_mutex_lock(&my_mutex);
            in_time ++;
            in_size += sz;
            pthread_mutex_unlock(&my_mutex);
        }
        else if(msg.type = HEARTBEAT){
            LOG("a heartbeat received\n");
            int cur_time = time((time_t*)NULL);
            if(cur_time - hb_time > 60){
                LOG("失去与服务器的连接，连接已断开");
                not_closed = 0;
                break;
            }
            hb_time = cur_time;
        }


    }

    close(client_socket);
}

int main(void){
    LOG("C starting");
    //初始化管道
    not_closed = 1;
    mknod(ip_name, 0666, 0);//创建ip管道
    ip_handle = open(ip_name, O_RDWR|O_CREAT|O_TRUNC);
    mknod(data_name, 0666, 0);//创建流量管道
    data_handle = open(data_name, O_RDWR|O_CREAT|O_TRUNC);

    //连接到服务器
    connect_to_server();

    //关闭管道
    close(ip_handle);
    close(data_handle);
}
JNIEXPORT void JNICALL Java_over6_over6client_MainActivity_stopCThread
        (JNIEnv *env, jobject thisz){
    not_closed = 0;
    close(ip_handle);
    close(data_handle);
    close(client_socket);
}

JNIEXPORT jstring JNICALL Java_over6_over6client_MainActivity_StringFromJNI
        (JNIEnv *env, jobject thisz){
    main();
    return (*env)->NewStringUTF(env,"hello from JNI");
}