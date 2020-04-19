#include "socket_api.hpp"

#include <arpa/inet.h>

void SampleTCPServer(const char* host, const char* port) {
  Socket socket_hd(PF_INET, SOCK_STREAM);
  socket_hd.EnbaleReuseAddress();
  socket_hd.Bind(host, atoi(port));
  socket_hd.Listen(5);

  size_t buf_size = 1024;
  char* buf = new char[buf_size];
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    int conn = socket_hd.Accept(reinterpret_cast<struct sockaddr*>(&client_addr), &socket_len);
    printf("Connection from %s:%d\n",
           inet_ntop(AF_INET, &client_addr.sin_addr, buf, INET_ADDRSTRLEN),
           ntohs(client_addr.sin_port));
    while (1) {
        int num_recv = recv(conn, buf, buf_size, 0);
        if (num_recv < 0) {
            printf("Error on Recv data from client!\n");
            break;
        }
        if (num_recv == 0) {
            close(conn);
            break;
        }
        send(conn, buf, num_recv, 0);
    }
  }
  free(buf);
}

void SampleTCPClient(const char* host, const char* port) {
  Socket socket_hd(PF_INET, SOCK_STREAM);
  socket_hd.Connect(host, atoi(port));

  size_t buf_size = 1024;
  char* buf = new char[buf_size];
  while (1) {
    // 可能会remalloc buf并重置buf_size
    ssize_t n = getline(&buf, &buf_size, stdin);
    buf[n - 1] = '\0';  // 去掉行尾的换行符

    // 设置client的退出机制
    if (strcmp("quit", buf) == 0) {
      break;
    }
    socket_hd.Send(buf, n);
  }
  free(buf);
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s <ip/hostname> <port>\n", basename(argv[0]));
    return -1;
  }

  SampleTCPServer(argv[1], argv[2]);

  //SampleTCPServer(argv[1], argv[2]);

  return 0;
}