//
// Created by yansheng on 2020/4/23.
//

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <cstdio>
#include <cstdlib>

#include "utils.h"
#include "socket_api.hpp"

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

  // 创建epoll句柄，并添加监听描述符
  const int32_t max_events_num = 10000;
  int epoll_hdl = epoll_create(max_events_num);
  if (epoll_hdl < 0) {
    exit_with_errno(epoll_hdl);
  }

  struct epoll_event event;
  struct epoll_event *event_set = new epoll_event[max_events_num];
  event.data.fd = socket_handle.GetSocketFD();
  event.events = EPOLLIN | EPOLLET;
  int ret = epoll_ctl(epoll_hdl, EPOLL_CTL_ADD, socket_handle.GetSocketFD(), &event);
  if (ret != 0) {
    printf("failed to add listened socket to epoll\n");
    exit_with_errno(ret);
  }

  const int32_t buf_size = 1024;
  char buf[buf_size];
  for(;;) {
    printf("Start to epoll wait\n");
    int32_t ready = epoll_wait(epoll_hdl, event_set, max_events_num, -1);
    printf("epoll wait return with:%d\n", ready);
    if (ready < 0) {
      printf("ready < 0, %s\n", gai_strerror(errno));
      break;
    }
    for (int32_t i = 0; i < ready; i++) {
      // 处理断开的连接
      // todo: !EPOLLIN 是不是包含了EPOLLRDHUP和EPOLLERR
      if (event_set[i].events & EPOLLRDHUP || event_set[i].events & EPOLLERR || !(event_set[i].events & EPOLLIN)) {
        // 正常的连接中断，会同时收到EPOLLIN和EPOLLRDHUP
        printf("Disconnected from %d\n", event_set[i].data.fd);
        close(event_set[i].data.fd);
        continue;
      }
      if (event_set[i].data.fd == socket_handle.GetSocketFD()) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(struct sockaddr_in);
        int conn = accept(event_set[i].data.fd, (struct sockaddr*)&client_addr, &addrlen);
        printf("New connection from %s:%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, buf, buf_size), ntohs(client_addr.sin_port));
        event.data.fd = conn;
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        epoll_ctl(epoll_hdl, EPOLL_CTL_ADD, conn, &event);
        continue;
      }
      int nread = recv(event_set[i].data.fd, buf, buf_size, 0);
      // 这里是不是不必要的
      if (nread == 0) {
        printf("One client connection closed\n");
        close(event_set[i].data.fd);
        continue;
      }
      send(event_set[i].data.fd, buf, nread, 0);
    }
  }

  close(epoll_hdl);

  return 0;
}