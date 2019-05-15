#ifndef UTIL_H
#define UTIL_H
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include<android/log.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <pthread.h>

#include "over6_over6client_MainActivity.h"


#define IP_REQ 100
#define IP_REP 101
#define DATA_SEND 102
#define DATA_RECV 103
#define HEARTBEAT 104

#define LOG(...) __android_log_print(ANDROID_LOG_DEBUG,"jni_C",__VA_ARGS__)

struct Msg {
    int length;		//整个结构体的字节长度
    char type;		//类型
    char data[4096];	//数据段
};


#endif