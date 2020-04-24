#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/epoll.h>

#include <cstdio>
#include <cstdlib>

#include "utils.h"
#include "socket_api.hpp"

// 通过信号来回收子进程
void signal_handler(int sig)
{
  // 这里为什么要用循环 ?
  // 一个发出而没有被接收的信号叫做待处理信号（pending signal）。
  // 在任何时刻，一种类型至多只会有一个待处理信号。
  // 如果一个进程有一个类型为k的待处理信号，那么任何接下来发送到这个进程的类型为k的信号都不会排队等待；
  // 它们只是被简单地丢弃。一个进程可以有选择性地阻塞接收某种信号。
  // 当一种信号被阻塞时，它仍可以被发送，但是产生的待处理信号不会被接收，直到进程取消对这种信号的阻塞。
  //一个待处理信号最多只能被接收一次。内核为每个进程在pending位向量中维护着待处理信号的集合，
  // 而在blocked位向量中维护着被阻塞的信号集合。只要传送了一个类型为k的信号，
  // 内核就会设置pending中的第k位，而只要接收了一个类型为k的信号，
  // 内核就会清除pending中的第k位。
  while (waitpid(-1, nullptr, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("Usage: %s <ip/hostname> <port>\n", basename(argv[0]));
    return -1;
  }

  signal(SIGCHLD, signal_handler);

  // 创建服务器端监听套接字
  Socket socket_handle(AF_INET, SOCK_STREAM);
  socket_handle.EnbaleReuseAddress();
  socket_handle.Bind(argv[1], atoi(argv[2]));
  socket_handle.Listen(5);

  struct sockaddr_in client_addr;
  socklen_t addrlen = sizeof(struct sockaddr_in);

  const int32_t buf_size = 1024;
  char buf[buf_size];

  for(;;) {
    int conn = socket_handle.Accept((struct sockaddr*)&client_addr, &addrlen);
    printf("New connection from %s:%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, buf, buf_size), ntohs(client_addr.sin_port));
    pid_t pid = fork();
    if (pid < 0) {
      printf("failed to call fork(): %s\n", gai_strerror(errno));
      return -1;
    }
    if (pid != 0) {
      // parent process
      close(conn);
      continue;
    }
    // else: child process
    for (;;) {
      int n_read = recv(conn, buf, buf_size, 0);
      if (n_read == 0) {
        printf("One client connection closed\n");
        close(conn);
        break;
      }
      send(conn, buf, n_read, 0);
    }
    break;
  }

  return 0;
}