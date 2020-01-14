#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>  //sockaddr_in
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 打印域名别名
static void print_alias_names(char **alias_names) {
  printf("alias names: [");
  int first_name = 1;
  for (char **alias_name = alias_names; *alias_name != NULL; alias_name++) {
    printf("%s%s", (first_name == 1 ? "" : ", "), *alias_name);
    first_name = 0;
  }
  printf("]\n");
}

// 打印ip地址列表
static void print_ip_address(char **ip_address, int addr_len) {
  printf("ip address: [");
  int first_name = 1;
  if (addr_len == sizeof(struct in_addr)) {
    // 打印ipv4地址的点分十进制
    char ip_string[INET_ADDRSTRLEN];
    for (char **ip = ip_address; *ip != NULL; ip++) {
      printf("%s%s", (first_name == 1 ? "" : ", "),
             inet_ntop(AF_INET, *ip, ip_string, INET_ADDRSTRLEN));
      first_name = 0;
    }
  } else if (addr_len == sizeof(struct in6_addr)) {
    // 打印ipv6地址的十六进制字符串形式
    char ip_string[INET6_ADDRSTRLEN];
    for (char **ip = ip_address; *ip != NULL; ip++) {
      printf("%s%s", (first_name == 1 ? "" : ", "),
             inet_ntop(AF_INET6, *ip, ip_string, INET6_ADDRSTRLEN));
      first_name = 0;
    }
  } else {
    printf("unknown ip address type!");
  }
  printf("]\n");
}

static void print_servent(const struct servent *serv_info) {
  printf("service name: %s\n", serv_info->s_name);
  printf("alias names: [");
  int first_name = 1;
  for (char **alias_name = serv_info->s_aliases; *alias_name != NULL; alias_name++) {
    printf("%s%s", (first_name == 1 ? "" : ", "), *alias_name);
    first_name = 0;
  }
  printf("]\n");
  printf("port number: %d\n", ntohs(serv_info->s_port));
  printf("protocol: %s\n", serv_info->s_proto);
}

int main(int argc, char *argv[]) {
  struct in_addr ipv4_address;
  int ret = inet_pton(AF_INET, "127.0.0.1", &ipv4_address);
  if (ret != 0) {
    printf("%s\n", strerror(errno));
  }
  struct hostent *host_info = gethostbyaddr(&ipv4_address, sizeof(struct in_addr), AF_INET);
  printf("hostname: %s\n", host_info->h_name);
  print_alias_names(host_info->h_aliases);
  print_ip_address(host_info->h_addr_list, host_info->h_length);
  // Output:
  // hostname: localhost
  // alias names: []
  // ip address: [127.0.0.1]

  host_info = gethostbyname("www.163.com");
  printf("hostname: %s\n", host_info->h_name);
  print_alias_names(host_info->h_aliases);
  print_ip_address(host_info->h_addr_list, host_info->h_length);
  // Output:
  // hostname: z163ipv6.v.bsgslb.cn
  // alias names: [www.163.com, www.163.com.163jiasu.com, www.163.com.bsgslb.cn]
  // ip address: [183.47.233.10, 183.47.233.9, 183.47.233.13, 183.47.233.6, 183.47.233.12,
  // 183.47.233.7, 183.47.233.14, 183.47.233.8, 183.47.233.11]

  struct servent *serv_info = getservbyname("https", "tcp");
  if (serv_info != NULL) {
    print_servent(serv_info);
  } else {
    printf("failed to get port by service name!\n");
  }
  // Output:
  // service name: https
  // alias names: []
  // port number: 443
  // protocol: tcp

  serv_info = getservbyport(htons(22), "tcp");
  if (serv_info != NULL) {
    print_servent(serv_info);
  } else {
    printf("failed to get service name by port number!\n");
  }
  // Output:
  // service name: ssh
  // alias names: []
  // port number: 22
  // protocol: tcp
  return 0;
}
