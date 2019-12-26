#include <netinet/in.h>
#include <cstdint>
#include <cstdio>

int main(int argc, char *argv[]) {
  union SockAddr {
    uint8_t bytes[4];
    uint32_t integer;
  };
  /********判断机器的字节序************/
  SockAddr sock_addr;
  sock_addr.integer = 0x12345678;
  // 将sock_addr按内存中的顺序，从低地址到高地址打印出4个字节的值
  printf("[%#x %#x %#x %#x]\n", sock_addr.bytes[0], sock_addr.bytes[1],
         sock_addr.bytes[2], sock_addr.bytes[3]);
  // [0x78 0x56 0x34 0x12]
  // 可以看出来低位(78)的放在了低字节，高位(12)的放在了高字节
  if (sock_addr.bytes[0] == 0x78 && sock_addr.bytes[3] == 0x12) {
    printf("little endian\n");
  } else {
    printf("Big endian\n");
  }
  // Output: little endian on Linux X86 Intel CPU PC

  /*******主机与网络字节序之间的转换*********/
  SockAddr local_addr;  // 127.0.0.1
  local_addr.bytes[0] = 1;
  local_addr.bytes[1] = 0;
  local_addr.bytes[2] = 0;
  local_addr.bytes[3] = 127;
  // 主机字节序，转换为网络字节序
  SockAddr net_addr;
  net_addr.integer = htonl(local_addr.integer);
  printf("netAddr: [%d %d %d %d]\n", net_addr.bytes[3], net_addr.bytes[2],
         net_addr.bytes[1], net_addr.bytes[0]);
  // Output: netAddr: [1 0 0 127]
  SockAddr host_addr;
  // 网络序再转主机序
  host_addr.integer = ntohl(net_addr.integer);
  printf("hostAddr: [%d %d %d %d]\n", host_addr.bytes[3], host_addr.bytes[2],
         host_addr.bytes[1], host_addr.bytes[0]);
  // Output: hostAddr: [127 0 0 1]
  return 0;
}