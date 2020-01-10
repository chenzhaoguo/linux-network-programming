#include <arpa/inet.h>  // inet_pton/inet_ntop
#include <stdio.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: %s <ip address>\n", argv[0]);
    return -1;
  }
  // 这个也是将字符串转化ip地址的函数，不过它支持IPv6
  in_addr_t ip;
  int ret = inet_pton(AF_INET, argv[1], &ip);
  if (INADDR_NONE == ret) {
    printf("Failed to translate the ip string: %s\n", argv[1]);
    return -1;
  }
  printf("integer ip address: %#x, return code: %d\n", ip, ret);
  // Output: integer ip address: 0x100007f, return code: 1

  // define INET_ADDSTRLEN 16
  // define INET6_ADDSTRLEN 46
  char str[INET_ADDRSTRLEN];
  const char* ret_str = inet_ntop(AF_INET, &ip, str, INET_ADDRSTRLEN);
  // 返回的地址就是传入的地址，如果失败，则返回NULL指针，并设置errno
  printf("%p vs. %p\n", str, ret_str);
  // Output: 0x7ffda541f700 vs. 0x7ffda541f700

  return 0;
}
