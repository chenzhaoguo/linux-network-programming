// 程序完成了服务器端远程执行客户端的命令
// 只支持部分命令
// 通过 chdir实现cd命令，由于cd本身不是可执行程序
// 其他命令都通过fork->execve来执行

// 实现细节
// 1. 客户端通过select来同时处理命令行输入与服务器的响应
// 避免了部分命令服务器不返回信息时，阻塞在recv操作上
// 2. 实现了将一行文本解析成一个个字符串的函数，功能简单，但实现的细节非常多
// 3. fork后子进程与父进程对于堆上存储的使用：CopyOnWrite
#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>  // basename
#include <netdb.h>   // gethostbyname

#include <ctype.h>  // isspace
#include <stdio.h>
#include <stdlib.h>  // atoi
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

void do_client(int sock_fd, const struct sockaddr_in *addr) {
  int ret = connect(sock_fd, (struct sockaddr *)addr, sizeof(*addr));
  if (ret != 0) {
    printf("[%d]: %s\n", errno, strerror(errno));
    return;
  }
  printf("Success to connect to server!\n");

  size_t buf_size = 1024000;
  char *buf = malloc(buf_size);

  fd_set fd_read_set;
  FD_ZERO(&fd_read_set);
  FD_SET(STDIN_FILENO, &fd_read_set);
  FD_SET(sock_fd, &fd_read_set);

  while (1) {
    fd_set fd_ready_set = fd_read_set;
    int n_read_ready = select(sock_fd + 1, &fd_ready_set, NULL, NULL, NULL);
    if (n_read_ready < 0) {
      exit_with_errno(n_read_ready);
    }
    if (FD_ISSET(STDIN_FILENO, &fd_ready_set)) {
      ssize_t n = getline(&buf, &buf_size, stdin);
      buf[n - 1] = '\0';  // 去掉行尾的换行符
      if (strcmp("quit", buf) == 0) {
        break;
      }
      send(sock_fd, buf, n, 0);
      continue;
    }
    // 如果不是前两种情况，则一定是sock_fd可读了
    // 这里没有再加　FD_ISSET(socd_fd, &fd_ready_set)检查
    memset(buf, 0, buf_size);
    ssize_t n_recv = recv(sock_fd, buf, buf_size, 0);
    if (n_recv == 0) {
      printf("lose connection from server!\n");
      break;
    }
    printf("%s", buf);
  }
  free(buf);
}

int split_str(const char *str, int n, char ***strs) {
  const int max_args = 5;
  *strs = (char **)malloc(sizeof(char *) * max_args);
  for (int i = 0; i < max_args; i++) {
    (*strs)[i] = (char *)malloc(1024);
  }

  char substr[1024];
  int j = 0;
  int k = 0;
  for (int i = 0; i < n && str[i] != '\0' && str[i] != '\n'; i++) {
    if (isspace(str[i])) {
      // 处理空白字符之前的子串
      substr[j] = '\0';
      //! fix the j -> j + 1
      strncpy((*strs)[k++], substr, j + 1);
      j = 0;
      // 跳过后续的空白
      while (isspace(str[i])) {
        i++;
      }
    }
    substr[j++] = str[i];
  }
  // 最后一个字符串的处理
  substr[j] = '\0';
  //! fix the j -> j + 1
  strncpy((*strs)[k++], substr, j + 1);

  for (int i = k; i < max_args; i++) {
    free((*strs)[i]);
    (*strs)[i] = NULL;
  }
  return k;
}

void handle_client_connection(int conn) {
  const uint32_t buf_size = 1024;
  char recv_buf[buf_size];
  while (1) {
    int n = recv(conn, recv_buf, buf_size, 0);
    if (n == 0) {
      break;
    }
    printf("Run Command: %s\n", recv_buf);
    // split_str里分配的存储，会随着子进程的退出，被os自动回收
    char **argv = NULL;
    int args = split_str(recv_buf, n, &argv);
    if (strncmp(argv[0], "cd", 2) == 0) {
      char send_buf[1024] = {0};
      if (chdir(argv[1]) != 0) {
        snprintf(send_buf, 1024, "Failed to change to %s\n", argv[1]);
      } else {
        snprintf(send_buf, 1024, "Successed to change to %s\n", argv[1]);
      }
      // strlen返回的长度不包括'\0',所以这里发送时长度+1, 保证接收方收到一个带结束标志的字符串
      send(conn, send_buf, strlen(send_buf) + 1, 0);
      continue;
    }
    // 关于fork后对于堆上的内存的管理问题
    // https://stackoverflow.com/questions/4597893/specifically-how-does-fork-handle-dynamically-allocated-memory-from-malloc
    if (fork() == 0) {
      close(STDOUT_FILENO);
      int newfd = dup(conn);
      if (execvp(argv[0], argv) < 0) {
        printf("Error commmands: [%d] %s\n", args, argv[0]);
      }
      exit(0);
    }
    // 等待子进程运行结束，不关心退出状态
    wait(NULL);
    for (int i = 0; i < args; i++) {
      free(argv[i]);
    }
    free(argv);
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
