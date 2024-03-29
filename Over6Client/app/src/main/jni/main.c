#include "over6_over6client_MainActivity.h"
#include "util.h"

const int MAX_BUFFER = 4105;

//管道相关
int ip_handle;
int data_handle;

int tun_descrip;

pthread_mutex_t my_mutex;

char ip_name[100] = "/data/data/over6.over6client/ip";
char data_name[100] = "/data/data/over6.over6client/data";
char ip_buffer[4105];
char SERVER_ADDR[100] = "2402:f000:4:72:808::6b04";
int  SERVER_PORT = 5678;

int client_socket;
int hb_time;

int not_closed = 1;
int in_time = 0;
int in_size = 0;
int out_time = 0;
int out_size = 0;


struct epoll_event events[5];


int write_handle(char name[],char msg[]){
     char b[1024] = "";
    mknod(name, 0666, 0);//创建ip管道
    int handle = open(name, O_RDWR|O_CREAT|O_TRUNC);
     bzero(b, sizeof(b));
     sprintf(b, "%s\0",msg);
     return write(handle, b, strlen(b) + 1);
}

int read_handle(char name[],char read_buffer[]){
     int handle = open(name, O_RDWR|O_CREAT);
     memset(read_buffer, 0, MAX_BUFFER*sizeof(char));
     int len = read(handle, read_buffer, MAX_BUFFER);
     close(handle);
     return len;
}

void* timer(){
    int ticks = 20;
    char sock_buffer[4105];
    struct Msg hb_msg;
    bzero(&hb_msg, sizeof(hb_msg));
    hb_msg.length = 8;
    hb_msg.type = HEARTBEAT;

    while(not_closed){
        int cur_time = time((time_t*)NULL);
        if(cur_time - hb_time > 60){
            LOG("失去与服务器的连接，连接已断开\n");
            write_handle(ip_name,"ERROR\n");
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
            memcpy(sock_buffer, &hb_msg, sizeof(struct  Msg));
            if(send(client_socket, sock_buffer,hb_msg.length, 0) <= 0){
                LOG("发送heartbeat失败\n");
            }else{
                LOG("发送heartbeat成功\n");
            }
            ticks = 20;
        }
        //流量统计
        char temp[100];
        bzero(temp,100);
        sprintf(temp,"%d %d %d %d ",in_time,in_size,out_time,out_size);
        LOG("%s\n",temp);
        write_handle(data_name,temp);
        sleep(1);
    }
    close(client_socket);
    client_socket = -1;
    LOG("timer exit");
    return 0;
}

void* data_send_thread(){
    LOG("vpn已启动%d\n",tun_descrip);
    char buffer[MAX_BUFFER + 1];
    bzero(buffer, MAX_BUFFER+1);
//    fd_set fd_s;
    int epfd = epoll_create(5);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = tun_descrip;
    epoll_ctl(epfd, EPOLL_CTL_ADD, tun_descrip, &ev);
    struct Msg msg;
    while(not_closed){
//        FD_ZERO(&fd_s);
//        FD_SET(tun_descrip ,&fd_s);
//         if(select(tun_descrip + 1, &fd_s, NULL, NULL, NULL) > 0) {
        if(epoll_wait(epfd, &events, 5, 500) > 0) {
//             if(!FD_ISSET(tun_descrip,&fd_s)){
//                 continue;
//             }
             //读文件描述符
            int length = read(tun_descrip, buffer, MAX_BUFFER);
            if(length == 0){
                continue;
            }else if(length == -1){
                LOG("TUN read ERROR\n");
                break;
            }
            msg.length = 8 + length;
            msg.type = DATA_SEND;
            memcpy(msg.data, buffer, length);
            memcpy(buffer, &msg, sizeof(struct Msg));
             //发送给服务器
            if(send(client_socket, buffer,  msg.length, 0) > 0){
                LOG("发送%d byte给服务器\n", msg.length);
                //修改发送次数和长度
                pthread_mutex_lock(&my_mutex);
                out_time++;
                out_size += msg.length;
                pthread_mutex_unlock(&my_mutex);
            }else{
                LOG("VPN 发送失败\n");
                break;
            }
             bzero(buffer, MAX_BUFFER+1);

        }
    }
    close(tun_descrip);
    LOG("vpn exit");
    return 0;
}

void connect_to_server(){
    int IP_got = 0;
    //创建socket连接
    if((client_socket = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
        write_handle(ip_name,"ERROR\n");
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
        write_handle(ip_name,"ERROR\n");
        LOG("连接服务器失败\n");
        return;
    }

    //发送ip请求
    struct Msg msg;
    bzero(&msg, sizeof(msg));
    msg.length = 8;
    msg.type = IP_REQ;
    char sock_buffer[4105];
    memcpy(sock_buffer, &msg, sizeof(struct  Msg));
    if(send(client_socket, sock_buffer, 8, 0) <= 0){
        LOG("发送ip请求失败");
    }

    hb_time = time((time_t*)NULL);

    pthread_mutex_init(&my_mutex, NULL);
    pthread_t timer_thread;
    pthread_t send_thread;

    //创建timer线程
    if(pthread_create(&timer_thread, NULL, timer, NULL) == -1){
        LOG("计时器线程创建失败!\n");
        return;
    }else{
        LOG("计时器线程创建成功!\n");
    }

    while(not_closed) {
        //判断连接是否已经断开
        int cur_time = time((time_t *) NULL);
        if (cur_time - hb_time > 60) {
            LOG("失去与主机的连接\n");
            not_closed = 0;
            break;
        }

        bzero(sock_buffer, MAX_BUFFER);

        recv(client_socket, sock_buffer, 4, 0);
        int i = 0;
        LOG("包长度%d\n",*(int *) sock_buffer);
        for (i = 0; i < *(int *) sock_buffer - 4; i++) {
            recv(client_socket, sock_buffer + 4 + i, 1, 0);
        }
        bzero(&msg, sizeof(struct Msg));
        memcpy(&msg, sock_buffer, sizeof(struct Msg));
        LOG("接收到%d\n", msg.type);
        //服务器回复的IP请求
        if (msg.type == IP_REP && IP_got == 0) {
            LOG("收到IP回复 %s\n",msg.data);
            IP_got = 1;
            memset(ip_buffer, 0, MAX_BUFFER);

            //发送到前端
            sprintf(ip_buffer,"%s%d ",msg.data,client_socket);
            write_handle(ip_name, ip_buffer);

            //从前端读取tun描述符
            sleep(1);
            memset(ip_buffer, 0, MAX_BUFFER);
            int len = read_handle(ip_name, ip_buffer);
            if (len <= 0) {
                write_handle(ip_name, "ERROR");
                LOG("读取tun描述符失败%s\n", ip_buffer);
                not_closed = 0;
                return;
            }
            //创建发送数据线程
            tun_descrip = atoi(ip_buffer);
            LOG("读取tun描述符成功：%d,%d\n", tun_descrip, client_socket);
            pthread_create(&send_thread, NULL, data_send_thread, NULL);

        }
        else if (msg.type == HEARTBEAT) {
            LOG("a heartbeat received\n");
            int cur_time = time((time_t *) NULL);
            if (cur_time - hb_time > 60) {
                write_handle(ip_name,"ERROR\n");
                LOG("失去与服务器的连接，连接已断开");
                not_closed = 0;
                break;
            }
            hb_time = cur_time;
        }
        else if (msg.type == DATA_RECV) {
            int sz = write(tun_descrip, msg.data, msg.length - 5);
            if (sz != msg.length - 5) {
                LOG("写入tun出错\n");
            } else {
                LOG("成功写入tun\n");
                pthread_mutex_lock(&my_mutex);
                in_time++;
                in_size += sz;
                pthread_mutex_unlock(&my_mutex);
            }
        }

    }
    pthread_mutex_destroy(&my_mutex);
    close(ip_handle);
    close(data_handle);
    close(client_socket);
    LOG("main thread exit");
}

int main(void){
    LOG("C starting");
    memset(ip_buffer,0,MAX_BUFFER);
    int len = read_handle(ip_name,ip_buffer);
    LOG("ip buffer:%s %d\n",ip_buffer,len);
    sscanf(ip_buffer,"%s %d",SERVER_ADDR,&SERVER_PORT);
    //初始化管道
    not_closed = 1;

    //连接到服务器
    connect_to_server();
    return 0;
}


JNIEXPORT jstring JNICALL Java_over6_over6client_MainActivity_StringFromJNI
        (JNIEnv *env, jobject thisz){
    in_time = 0;
    in_size = 0;
    out_time = 0;
    out_size = 0;
    main();
    write_handle(ip_name,"ERROR\n");
    return (*env)->NewStringUTF(env,"hello from JNI");
}