#include "util.h"

//写入管道给java
int write_tunnel(char* fifo_name,char* buf){
    int fifo_handle;
    char fifo_buf[100] = "";
    int size;
    mknod(fifo_name, 0666, 0);//创建有名管道
    fifo_handle = open(fifo_name, O_RDWR|O_CREAT|O_TRUNC);
    size = write(fifo_handle, buf, sizeof(buf));
    close(fifo_handle);

}