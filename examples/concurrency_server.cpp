#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "socket_api.hpp"

// ip str / hostname to int
// construct the sock_addr_in4

// create socket
// bind address
// start to listen

// add sock_fd to epoll list

// for-loop:
//    if event fd is listen f: add connected fd to epoll list
//    for loop:
//        read data from ready fd
//        write data to fd
//        if recv return -1, close the fd, remove fd from epool list

int main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("Usage: %s <ip/hostname> <port>\n", basename(argv[0]));
    return -1;
  }

  // 创建服务器端监听套接字
  Socket socket_handle(AF_INET, SOCK_STREAM);
  socket_handle.EnbaleReuseAddress();
  socket_handle.Bind(argv[1], atoi(argv[2]));
  socket_handle.Listen(5);

  // 创建poll监听集合
  const int32_t max_fds = 1000;
  struct pollfd *fds = (struct pollfd*)malloc(sizeof(struct pollfd) * max_fds);
  for (int32_t i = 0; i < max_fds; i++) {
    fds[i].fd = -1;
    fds[i].events = 0;
    fds[i].revents = 0;
  }
  // 将服务器监听套接字加入poll集合中，用于监听新的连接
  fds[0].fd = socket_handle.GetSocketFD();
  fds[0].events = fds[0].events | POLLIN;

  const int32_t buf_size = 1024;
  char buf[buf_size];
  for(;;) {
    int32_t ready = poll(fds, max_fds, -1);
    if (ready < 0) {
      // pool返回负数，则代表出错，错误类型写在errno里
      printf("ready < 0, %s\n", gai_strerror(errno));
      break;
    }
    if (fds[0].revents & POLLIN) {
      struct sockaddr_in client_addr;
      socklen_t addrlen = sizeof(struct sockaddr_in);
      int conn = accept(fds[0].fd, (struct sockaddr*)&client_addr, &addrlen);
      printf("New connection from %s:%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, buf, buf_size), ntohs(client_addr.sin_port));
      // 在集合中找一个没有占用的位置
      for (int32_t i = 1; i < max_fds; i++) {
        if (fds[i].fd  == -1) {
          fds[i].fd = conn;
          fds[i].events = fds[i].events | POLLIN;
          // 找到后就可以退出了
          break;
        }
      }
      // 如果poll响应的事件都处理了，则可以继续下一轮poll了，避免后续的循环
      if (--ready == 0) {
        continue;
      }
    }
    // 继续在poll中查询IO事件
    for (int32_t i = 1; i < max_fds; i++) {
      if (fds[i].revents & POLLIN) {
        --ready;
        int nread = recv(fds[i].fd, buf, buf_size, 0);
        if (nread == 0) {
          printf("One client connection closed\n");
          close(fds[i].fd);
          fds[i].fd = -1;
          continue;
        }
        send(fds[i].fd, buf, nread, 0);
      }
      // 如果这次所有的IO事件都被处理了，则跳出该层for-loop，继续下一次的poll
      if (ready == 0) {
        break;
      }
    }
  }

  return 0;
}