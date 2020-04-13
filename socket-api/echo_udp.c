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

#include "utils.h"

void do_client(int sock_fd, const struct sockaddr_in *addr) {
  // connect可以在udp socket上调用，这样就可以使用send代替sendto
  int ret = connect(sock_fd, (struct sockaddr *)addr, sizeof(*addr));
  if (ret != 0) {
    printf("[%d]: %s\n", errno, strerror(errno));
    return;
  }

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
    buf[n - 1] = '\0';  // 去掉行尾的换行符

    // 设置client的退出机制
    if (strcmp("quit", buf) == 0) {
      break;
    }
    int nsent = send(sock_fd, buf, n, 0);
    if (nsent < 0) {
      exit_with_errno(nsent);
    }
    printf("%d bytes data sent to server\n", nsent);
    // 从服务端接收数据
    const uint32_t recv_buf_size = 1024;
    char recv_buf[recv_buf_size];
    memset(recv_buf, '\0', recv_buf_size);
    struct sockaddr_in server_addr;
    socklen_t server_addrlen;
    ssize_t n_recv = recv(sock_fd, recv_buf, recv_buf_size, 0);
    if (n_recv < 0) {
      exit_with_errno(n_recv);
    }
    printf("Recv: [%ld] %s\n", n_recv, recv_buf);
  }
  free(buf);
}

void do_server(int sock_fd, const struct sockaddr_in *addr) {
  int ret = bind(sock_fd, (struct sockaddr *)(addr), sizeof(*addr));
  if (ret != 0) {
    printf("[%d] %s\n", errno, strerror(errno));
    return;
  }
  // udp socket不支持listen与accept
  while (1) {
    const uint32_t buf_size = 1024;
    char recv_buf[buf_size];

    struct sockaddr_in client_addr;
    ///!important 这里必须要给add_len赋值
    uint32_t addr_len = sizeof(client_addr);
    // Datagram sockets in various domains (e.g., the UNIX and Internet domains)
    // permit zero-length datagrams. When such a datagram is received, the return value is 0.
    int n = recvfrom(sock_fd, recv_buf, buf_size, 0, (struct sockaddr*)&client_addr, &addr_len);
    if (n < 0) {
      break;
    }
    char client_ip[INET_ADDRSTRLEN];
    memset(client_ip, '\0', INET_ADDRSTRLEN);
    printf("Connection from %s:%d\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN),
        ntohs(client_addr.sin_port));
    printf("recv: [%d] %s\n", n, recv_buf);

    sendto(sock_fd, recv_buf, n, 0, (struct sockaddr*)&client_addr, addr_len);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Usage: %s <role>(server/client) <ip/hostname> <port>\n", basename(argv[0]));
    return -1;
  }

  // 根据程序参数来生成socket的地址
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv[3]));  // 端口号也需要转换为网络字节序
  // 根据hostname（点分十进制/域名）获取ip地址（网络字节序）
  struct hostent *he = gethostbyname(argv[2]);
  memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

  // 创建socket
  int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);

  if (strcmp(argv[1], "server") == 0) {
    do_server(sock_fd, &addr);
  } else if (strcmp(argv[1], "client") == 0) {
    do_client(sock_fd, &addr);
  } else {
    printf("Usage: %s <role>(server/client) <ip/hostname> <port>\n", basename(argv[0]));
    return -1;
  }

  close(sock_fd);

  return 0;
}