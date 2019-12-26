#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
  const char *ipstr = "127.0.0.1";
  // 将点分十进制ipv4的地址转化为一个网络字节序的整数
  in_addr_t in4_addr = inet_addr(ipstr);
  union {
    uint32_t integer;
    uint8_t bytes[4];
  } transfer;
  transfer.integer = in4_addr;
  // 我们按照大端字节序来打印出来
  printf("[%d %d %d %d]\n", transfer.bytes[0], transfer.bytes[1],
         transfer.bytes[2], transfer.bytes[3]);
  // Output: [127 0 0 1]

  // 将地址直接写到一个分配好的内址地址里
  struct in_addr alloc_in4;
  // 特别注意：成功返回1，失败返回0
  int ret = inet_aton("299.0.0.1", &alloc_in4);
  printf("result: %d\n", ret);
  // Output: result: 0

  // 将一个网络字节序的地址，转换为点分十进制的
  transfer.bytes[0] = 10;
  transfer.bytes[1] = 5;
  transfer.bytes[2] = 36;
  transfer.bytes[3] = 72;
  // 需要注意：inet_ntoa函数内部用一个静态变量存储转换的结果，函数的返回值
  // 指向该静态内存，因此inet_ntoa是不可重入的。
  printf("%s\n", inet_ntoa(*(in_addr *)(&transfer.integer)));
  // Output: 10.5.36.72

  /**********新型的两个转换函数，支持IPv6************/

  // 这个也是将字符串转化ip地址的函数，不过它支持IPv6
  auto err = inet_pton(AF_INET, "127.0.0.1", &transfer.integer);
  printf("[%d %d %d %d]\n", transfer.bytes[0], transfer.bytes[1],
         transfer.bytes[2], transfer.bytes[3]);
  // Output: [127 0 0 1]

  char str[INET_ADDRSTRLEN];
  auto ret_str = inet_ntop(AF_INET, &transfer.integer, str, INET_ADDRSTRLEN);
  printf("%s vs. %s\n", str, ret_str);
  // Output: 127.0.0.1
  // 返回的地址就是传入的地址
  printf("%p vs. %p\n", str, ret_str);
  // Output: 0x7ffd97a54380 vs. 0x7ffd97a54380

  return 0;
}