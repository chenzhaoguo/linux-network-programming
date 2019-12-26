#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>  // basename
#include <netdb.h>   // gethostbyname
#include <stdio.h>
#include <stdlib.h>  // atoi
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <ip/hostname> <port>\n", basename(argv[0]));
        return -1;
    }

    // 根据程序参数来生成socket的地址
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));  // 端口号也需要转换为网络字节序
    // 根据hostname（点分十进制/域名）获取ip地址（网络字节序）
    struct hostent *he = gethostbyname(argv[1]);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    // 创建socket
    int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    int ret = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0) {
        printf("[%d]: %s\n", errno, strerror(errno));
        return -1;
    }
    printf("Success to connect to server!\n");

    size_t buf_size = 1024;
    char *buf = malloc(buf_size);

    while (1) {
        // 从标准输入中读取一行
        // getline会在换行符'\n'(10)后面，再补一个'\0'(0)
        // 所以不用担心buf里有脏数据导致strlen计算错误的问题
        // 返回的n是算上换行符
        // 如果一行文本的内容大于buf_size，那么getline会realloc
        // 同时更新buf和buf_size的值
        ssize_t n = getline(&buf, &buf_size, stdin);
        buf[n - 1] = '\0'; // 去掉行尾的换行符

        // 设置client的退出机制
        if (strcmp("quit", buf) == 0) {
            break;
        }
        char *large_buf = malloc(102400);
        // 往服务端发送数据，并接收服务端的返回数据
        send(sock_fd, large_buf, 102400, 0);

        // 从服务端接收数据
        const uint32_t recv_buf_size = 1024;
        char recv_buf[recv_buf_size];
        memset(recv_buf, '\0', recv_buf_size);
        ssize_t n_recv = recv(sock_fd, recv_buf, recv_buf_size, 0);
        printf("Recv: [%d] %s\n", n_recv, recv_buf);
    }
    free(buf);
    close(sock_fd);

    return 0;
}