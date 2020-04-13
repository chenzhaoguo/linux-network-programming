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

// 客户端与服务器之间通信的数据格式
typedef struct {
  uint32_t type; // 消息类型
  char data[1024]; // 消息内容
} messageObject;

typedef enum {
  MSG_PING = 0,
  MSG_PONG = 1,
  MSG_TYPE1 = 2,
  MSG_TYPE3 = 3
} messageType;

void do_client(int sock_fd, const struct sockaddr_in *addr) {
  const int KEEP_ALIVE_TIME = 10;
  const int KEEP_ALIVE_INTERVA = 3;
  const int KEEP_ALIVE_PROBETIMES = 3;
  int ret = connect(sock_fd, (struct sockaddr *)addr, sizeof(*addr));
  if (ret != 0) {
    printf("[%d]: %s\n", errno, strerror(errno));
    return;
  }
  printf("Success to connect to server!\n");

  size_t buf_size = 1024;
  char *buf = malloc(buf_size);

  struct timeval tv;
  tv.tv_sec = KEEP_ALIVE_TIME;
  tv.tv_usec = 0;
  int heartbeats = 0;

  fd_set readmask;
  fd_set allreads;
  FD_ZERO(&allreads);
  FD_SET(sock_fd, &allreads);

  messageObject msg_obj;

  while (1) {
    readmask = allreads;
    int rc = select(sock_fd + 1, &readmask, NULL, NULL, &tv);
    // 出错
    if (rc < 0) {
      exit_with_errno(rc);
    }
    // 超时
    if (rc == 0) {
      if (++heartbeats > KEEP_ALIVE_PROBETIMES) {
        printf("connection dead!\n");
        exit_with_errno(0);
      }
      printf("sending heartbeat #%d\n", heartbeats);
      msg_obj.type = htonl(MSG_PING);
      rc = send(sock_fd, &msg_obj, sizeof(msg_obj), 0);
      if (rc < 0) {
        exit_with_errno(rc);
      }
      tv.tv_sec = KEEP_ALIVE_INTERVA;
      continue;
    }
    // 正常数据
    if (FD_ISSET(sock_fd, &readmask)) {
      int n = read(sock_fd, buf, buf_size);
      if (n < 0) {
        exit_with_errno(n);
      }
      if (n == 0) {
        printf("server closed!\n");
        exit_with_errno(0);
      }
      printf("received heartbeat, make heartbeats to 0\n");
      heartbeats = 0;
      tv.tv_sec = KEEP_ALIVE_TIME;
    }
  }
  free(buf);
}

void handle_client_connection(int conn) {
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

void do_server(int sock_fd, const struct sockaddr_in *addr) {
  int on = 1;
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  int ret = bind(sock_fd, (struct sockaddr *)(addr), sizeof(*addr));
  if (ret != 0) {
    printf("[%d] %s\n", errno, strerror(errno));
    return;
  }

  // 监听socket
  ret = listen(sock_fd, 5);
  if (ret != 0) {
    printf("[%d] %s\n", errno, strerror(errno));
    return;
  }
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    int conn = accept(sock_fd, (struct sockaddr *)(&client_addr), &addr_len);
    if (conn == -1) {
      printf("[%d] %s\n", errno, strerror(errno));
      return;
    }
    char client_ip[INET_ADDRSTRLEN];
    memset(client_ip, '\0', INET_ADDRSTRLEN);
    printf("Connection from %s:%d\n",
           inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN),
           ntohs(client_addr.sin_port));
    handle_client_connection(conn);
    printf("Connection closed!\n");
    close(conn);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
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
  int sock_fd = socket(PF_INET, SOCK_STREAM, 0);

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