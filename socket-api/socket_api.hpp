#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

#include <cstdint>
#include <string>

static int GetAddressByName(const std::string& name, int16_t port, int pf,
                            struct sockaddr* address) {
  int ret = 0;
  switch (pf) {
    case PF_UNIX: {
      struct sockaddr_un* local_address = reinterpret_cast<struct sockaddr_un*>(address);
      local_address->sun_family = AF_UNIX;
      strcpy(local_address->sun_path, name.c_str());
      break;
    }
    case PF_INET: {
      struct hostent host_info;
      struct hostent* return_host_info = NULL;
      char buf[1024];
      int errcode = 0;
      int ret = gethostbyname2_r(name.c_str(), AF_INET, &host_info, buf, 1024, &return_host_info,
                                 &errcode);
      if (ret != 0) {
        printf("%s return non-zero value = %d, cause: %s\n", "gethostbyname2_r", ret,
               gai_strerror(errcode));
        return -1;
      }
      struct sockaddr_in* address_in = reinterpret_cast<struct sockaddr_in*>(address);
      address_in->sin_family = AF_INET;
      memcpy(&address_in->sin_addr, return_host_info->h_addr_list[0], return_host_info->h_length);
      address_in->sin_port = htons(port);
      break;
    }
    case PF_INET6: {
      printf("Not Implemented!\n");
      ret = -1;
      break;
    }
    default: {
      ret = -1;
      break;
    }
  }

  return ret;
}

class Socket {
 public:
  Socket(int domain, int type) {
    domain_ = domain;
    socket_fd_ = socket(domain, type, 0);
  }

  ~Socket() { close(socket_fd_); }

  int Bind(const std::string& hostname, int16_t port) {
    struct sockaddr* universal_address =
        reinterpret_cast<struct sockaddr*>(malloc(sizeof(struct sockaddr_storage)));
    int ret = GetAddressByName(hostname, port, domain_, universal_address);
    if (ret != 0) {
      return -1;
    }
    ret = bind(socket_fd_, universal_address, sizeof(struct sockaddr_storage));
    if (ret != 0) {
      printf("%s return non-zero value = %d, cause: %s\n", "bind", ret, gai_strerror(errno));
      return -1;
    }
    free(universal_address);
    return 0;
  }

  int Listen(int backlog) {
    int ret = listen(socket_fd_, backlog);
    if (ret != 0) {
      printf("%s return non-zero value = %d, cause: %s\n", "listen", ret, gai_strerror(errno));
      return -1;
    }

    return 0;
  }

  int Connect(const std::string& server, int16_t port) {
    struct sockaddr* universal_address =
        reinterpret_cast<struct sockaddr*>(malloc(sizeof(struct sockaddr_storage)));
    int ret = GetAddressByName(server, port, domain_, universal_address);
    if (ret != 0) {
      return -1;
    }
    ret = connect(socket_fd_, universal_address, sizeof(struct sockaddr_storage));
    if (ret != 0) {
      printf("%s return non-zero value = %d, cause: %s\n", "connect", ret, gai_strerror(errno));
      return -1;
    }
    free(universal_address);
    return 0;
  }

  int Accept(struct sockaddr* client_address, socklen_t* socket_len) {
    return accept(socket_fd_, client_address, socket_len);
  }

  void EnbaleReuseAddress() {
    int on = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  }

  int GetSocketFD() const { return socket_fd_; }

  int Send(const char* buf, int buf_size) { return send(socket_fd_, buf, buf_size, 0); }

  int Recv(char* buf, int buf_size) { return recv(socket_fd_, buf, buf_size, 0); }

 private:
  int socket_fd_ = -1;
  int domain_ = 0;
};