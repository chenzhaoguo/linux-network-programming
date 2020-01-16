/*
该程序主要演示了对于`dup`系统调用的使用
通过关闭标准输出(STDOUT_FILENO)，
使得dup将当前最小的文件描述符号(1)也指向了打开的文件
这样通过printf往标准输出上输出的内容就会保存在文件中了
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils.h"

int main(int argc, char *argv[]) {
    // O_CREAT　如果文件不存在，则会创建一个新的
    // 0664: -rw,-rw,r--
    int file_fd = open("/tmp/duplog.txt", O_CREAT | O_RDWR, 0664);
    if (-1 == file_fd) {
        exit_with_errno(-1);
    }
    close(STDOUT_FILENO);
    int newfd = dup(file_fd);
    printf("new fd = %d, testing dup system call!\n", newfd);
    close(file_fd);
    return 0;
}