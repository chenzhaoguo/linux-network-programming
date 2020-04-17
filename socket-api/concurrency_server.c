#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

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
  struct hostent host_info;
  struct hostent *return_host_info = NULL;
  char temp_buf[1024];
  int errcode = 0;
  int ret = gethostbyname_r(argv[1], &host_info, temp_buf, 1024, &return_host_info, &errcode);
  if (ret != 0) {
    // 这里出错了，实际是要看errcode的，而不是errno
    exit_with_errno(ret);
  }
  printf("IP: %s\n", inet_ntop(AF_INET, return_host_info->h_addr_list[0], temp_buf, INET_ADDRSTRLEN));
  // 当域名对应多条IP地址时，就会解析出来多条地址
  //printf("IP: %s\n", inet_ntop(AF_INET, return_host_info->h_addr_list[1], temp_buf, INET_ADDRSTRLEN));

  struct sockaddr_in server_ipv4_addr;
  memset(&server_ipv4_addr, 0, sizeof(server_ipv4_addr));
  server_ipv4_addr.sin_family = AF_INET;
  memcpy(&server_ipv4_addr.sin_addr.s_addr, return_host_info->h_addr_list[0], return_host_info->h_length);
  //server_ipv4_addr.sin_addr.s_addr = *return_host_info->h_addr_list[0];
  server_ipv4_addr.sin_port = htons(atoi(argv[2]));

  printf("%d\n", server_ipv4_addr.sin_addr.s_addr);

  struct hostent *he = gethostbyname(argv[2]);
  memcpy(&server_ipv4_addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
  printf("%d\n", server_ipv4_addr.sin_addr.s_addr);

  int sock = socket(PF_INET, SOCK_STREAM, 0);
  int on = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  printf("Start to bind!\n");
  ret = bind(sock, (struct sockaddr*)&server_ipv4_addr, sizeof(struct sockaddr_in));
  if (ret != 0) {
    exit_with_errno(ret);
  }
  printf("Start to listen!\n");
  const int32_t backlog = 5;
  ret = listen(sock, backlog);
  if (ret != 0) {
    exit_with_errno(ret);
  }


  const int32_t max_fds = 1000;
  struct pollfd *fds = (struct pollfd*)malloc(sizeof(struct pollfd) * max_fds);
  for (int32_t i = 0; i < max_fds; i++) {
    fds[i].fd = -1;
    fds[i].events = 0;
    fds[i].revents = 0;
  }
  fds[0].fd = sock;
  fds[0].events = fds[0].events | POLLIN;

  const int32_t read_buf_size = 1024;
  char read_buf[read_buf_size];

  for(;;) {
    printf("start poll POLLIN\n");
    int32_t ready = poll(fds, max_fds, -1);
    if (ready < 0) {
      // pool返回负数，则代表出错，错误类型写在errno里
      printf("%s\n", strerror(errno));
      break;
    }
    printf("ready = %d\n", ready);

    if (fds[0].revents & POLLIN) {
      struct sockaddr_in client_addr;
      int addrlen = 0;
      printf("Accept client connection\n");
      int conn = accept(fds[0].fd, (struct sockaddr*)&client_addr, &addrlen);
      for (int32_t i = 1; i < max_fds; i++) {
        if (fds[i].fd  == -1) {
          fds[i].fd = conn;
          fds[i].events = fds[i].events | POLLIN;
          break;
        }
      }
      if (--ready == 0) {
        continue;
      }
    }
    for (int32_t i = 1; i < max_fds; i++) {
      if (fds[i].revents & POLLIN) {
        int ret = recv(fds[i].fd, read_buf, read_buf_size, 0);
        if (ret < 0) {
          fds[i].fd = -1;
        }
        send(fds[i].fd, read_buf, read_buf_size, 0);
      }
      if (--ready == 0) {
        break;
      }
    }
  }







  close(sock);

  return 0;
}