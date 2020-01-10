#include <netinet/in.h>  // htonl
#include <stdint.h>      // uint8_t/uint32_t
#include <stdio.h>

int main(int argc, char *argv[]) {
  typedef union {
    uint8_t bytes[4];
    uint32_t integer;
  } integer_union;

  /********判断机器的字节序************/
  integer_union union_number;
  union_number.integer = 0x12345678;
  // 将sock_addr按内存中的顺序，从低地址到高地址打印出4个字节的值
  printf("Low-Address %#x %#x %#x %#x High-Address\n", union_number.bytes[0],
         union_number.bytes[1], union_number.bytes[2], union_number.bytes[3]);
  // Low-Address 0x78 0x56 0x34 0x12 High-Address
  // 可以看出来低位(78)的放在了低字节，高位(12)的放在了高字节
  if (union_number.bytes[0] == 0x78 && union_number.bytes[3] == 0x12) {
    printf("little endian\n");
  } else {
    printf("Big endian\n");
  }
  // Output: little endian (Linux X86 Intel CPU PC)

  /*******主机字节序，转换为网络字节序*********/
  int32_t host_integer = 0x01234578;
  printf("host integer: %#x\n", host_integer);
  // Output: host integer: 0x1234578
  int32_t net_integer = htonl(host_integer);
  printf("network integer: %#x\n", net_integer);
  // Output: network integer: 0x78452301

  return 0;
}