#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static bool stop = false;

static void handle_sig(int sig) { stop = true; }

int main(int argc, char *argv[]) {
  // 注册信号
  signal(SIGTERM, handle_sig);

  int sock_fd = socket(PF_INET, SOCK_STREAM, 0);

  sockaddr_un a;

  const char *local_ip = "172.20.25.122";

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(23334);  // 端口号也需要转换为网络字节序
  int ret = inet_pton(AF_INET, local_ip, &addr.sin_addr);
  if (ret != 1) {
    printf("failed parsing the ip addr: %s\n", local_ip);
    return -1;
  }
  // 由于绑定了一个较小的端口号，所以会失败
  ret = bind(sock_fd, (struct sockaddr *)(&addr), sizeof(addr));
  if (ret != 0) {
    printf("[%d] %s\n", errno, strerror(errno));
    return -1;
  }
  // Output: [13] failed to bind the socket to ip :Permission denied

  ret = listen(sock_fd, 5);
  if (ret != 0) {
    printf("[%d] %s\n", errno, strerror(errno));
    return -1;
  }

  sleep(10);

  sockaddr_in client_addr;
  uint32_t addr_len;
  int conn = accept(sock_fd, (sockaddr *)(&client_addr), &addr_len);
  if (conn == -1) {
    printf("[%d] %s\n", errno, strerror(errno));
    return -1;
  }
  char client_ip[INET_ADDRSTRLEN];
  printf("Connection from %s:%d\n",
         inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN),
         ntohs(client_addr.sin_port));
  close(conn);

  close(sock_fd);

  return 0;
}