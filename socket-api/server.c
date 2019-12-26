#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>  // basename
#include <netdb.h>   // gethostbyname
#include <signal.h>  // signal
#include <stdio.h>
#include <stdlib.h>  // atoi
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int32_t stop = 0;

static void handle_sig(int sig) { stop = 1; }

void HandleClientConnection(int conn) {
    const uint32_t buf_size = 1024;
    char recv_buf[buf_size];
    while (1) {
        int n = recv(conn, recv_buf, buf_size, 0);
        if (n == 0) {
            break;
        }
        printf("recv: [%d] %s\n", n, recv_buf);
        send(conn, recv_buf, n, 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <ip/hostname> <port>\n", basename(argv[0]));
        return -1;
    }
    // 注册信号，由于Server会处理Accpet的死循环
    // 通过监听外部中断信号来优雅的中断
    signal(SIGTERM, handle_sig);

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
    int ret = bind(sock_fd, (struct sockaddr *)(&addr), sizeof(addr));
    if (ret != 0) {
        printf("[%d] %s\n", errno, strerror(errno));
        return -1;
    }

    // 监听socket
    ret = listen(sock_fd, 5);
    if (ret != 0) {
        printf("[%d] %s\n", errno, strerror(errno));
        return -1;
    }
    while (!stop) {
        struct sockaddr_in client_addr;
        uint32_t addr_len;
        int conn = accept(sock_fd, (struct sockaddr *)(&client_addr), &addr_len);
        if (conn == -1) {
            printf("[%d] %s\n", errno, strerror(errno));
            return -1;
        }
        char client_ip[INET_ADDRSTRLEN];
        printf("Connection from %s:%d\n",
               inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,
                         INET_ADDRSTRLEN),
               ntohs(client_addr.sin_port));
        HandleClientConnection(conn);
        printf("Connection closed!\n");
        close(conn);
    }

    printf("Server Exit!\n");

    close(sock_fd);

    return 0;
}