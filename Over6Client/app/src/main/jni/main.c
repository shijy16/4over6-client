#include "over6_over6client_MainActivity.h"
#include "util.h"
const int MAX_BUFFER = 4104;

//管道相关
int fifo_handle;
int data_handle;
char* ip_name = "/data/data/over6.over6client/ip";
char* data_name = "/data/data/over6.over6client/data";
char read_buffer[4104];

int write_handle(int handle,char* msg){
    char b[1024] = "";
    bzero(b, sizeof(b));
    sprintf(b, "%s\0",msg);
    return write(handle, b, strlen(b) + 1);
}

int read_handle(char* name){
    int handle = open(name, O_RDWR|O_CREAT);
    memset(read_buffer, 0, MAX_BUFFER*sizeof(char));
    int len = read(handle, read_buffer, MAX_BUFFER);
    close(handle);
    return len;
}

int main(void){
    LOG("C starting");
    //初始化管道
    mknod(ip_name, 0666, 0);//创建ip管道
    fifo_handle = open(ip_name, O_RDWR|O_CREAT|O_TRUNC);
    mknod(data_name, 0666, 0);//创建流量管道
    data_handle = open(data_name, O_RDWR|O_CREAT|O_TRUNC);

    write_handle(fifo_handle,"I'm msg sent by shijy16 with c.\n");
    write_handle(fifo_handle,"I'm second msg sent by shijy16 with c.\n");
    sleep(3);
    if(read_handle(data_name) > 0){
        LOG("succeed to receive");
    }else{
        LOG("failed to receive");
    }

    //关闭管道
    close(fifo_handle);
}

JNIEXPORT jstring JNICALL Java_over6_over6client_MainActivity_StringFromJNI
        (JNIEnv *env, jobject thisz){
    main();
    return (*env)->NewStringUTF(env,"hello from JNI");
}