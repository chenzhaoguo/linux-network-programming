#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <libgen.h> // basename
#include <stdlib.h> // atoi

static int stop = 0;

static void handle_sig(int sig) { stop = 1; }

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <ip> <port>\n", basename(argv[0]));
    return -1;
  }
  // 注册信号
  signal(SIGTERM, handle_sig);

  // 创建socket句柄
  int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
  // 构建server侧的socket地址
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(argv[2]));  // 端口号也需要转换为网络字节序
  int ret = inet_pton(AF_INET, argv[1], &address.sin_addr);
  if (ret != 1) {
    printf("failed parsing the ip address: %s\n", argv[1]);
    return -1;
  }
  // 由于绑定了一个较小的端口号，所以会失败
  ret = bind(sock_fd, (struct sockaddr *)(&address), sizeof(address));
  if (ret != 0) {
    printf("[%d] %s\n", errno, strerror(errno));
    return -1;
  }
  ret = listen(sock_fd, 5);
  if (ret != 0) {
    printf("[%d] %s\n", errno, strerror(errno));
    return -1;
  }
  // 只接受连接，但不处理连接
  while (!stop) {
    sleep(1);
  }
  close(sock_fd);

  return 0;
}