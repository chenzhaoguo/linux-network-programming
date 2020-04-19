#include <stdio.h>
#include <unistd.h>
#include "utils.h"


#define RD_FD 0 // 读端的文件描述符序号
#define WR_FD 1 // 写端的文件描述符序号

int main(int argc, char *argv[]) {
    int fd[2];
    int ret = pipe(fd);
    if (ret != 0) {
        exit_with_errno(-1);
    }
    int pid = fork();
    if (pid == -1) {
        exit_with_errno(-1);
    }
    if (pid == 0) {
        // 子进程
        close(fd[WR_FD]);
        char read_buf[1024];
        int n_read = read(fd[RD_FD], read_buf, 1024);
        printf("%s\n", read_buf);
    }
    const char *greetings = "hello, world";
    close(fd[RD_FD]);
    // strlen是不计算字符串最后的'\0'的
    int n_writen = write(fd[WR_FD], greetings, strlen(greetings) + 1);

    return -1;
}