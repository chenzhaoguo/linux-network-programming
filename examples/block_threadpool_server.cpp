#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <cstdio>
#include <cstdlib>
#include <thread>
#include <functional>

#include "utils.h"
#include "socket_api.hpp"
#include "block_queue.hpp"



class ThreadPool {
 public:
  explicit ThreadPool(int32_t pool_size, std::function<void(BlockQueue<int>&)> &f, BlockQueue<int> &task_queue) {
    for (int i = 0; i < pool_size; i++) {
      thread_pools.emplace_back(std::thread(f, std::ref(task_queue)));
    }
  }

  ~ThreadPool() {
    for (auto &t : thread_pools) {
      t.join();
    }
  }
 private:
  std::vector<std::thread> thread_pools;
};

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