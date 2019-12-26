#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <libgen.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage %s <hostanme>\n", basename(argv[0]));
        return -1;
    }
    // 根据主机名，获取主机的地址
    char * hostname = argv[1];
    struct hostent *he = gethostbyname(hostname);

    // 打印别名
    char **alias_name = NULL;
    for (alias_name = he->h_aliases; *alias_name != NULL; alias_name++) {
        printf("Alias name: %s\n", *alias_name);
    }

    char buf[INET_ADDRSTRLEN];
    char **ip_addr = he->h_addr_list;
    while (*ip_addr != NULL) {
        printf("ip: %s\n", inet_ntop(he->h_addrtype, *ip_addr, buf, INET_ADDRSTRLEN));
        ip_addr++;
    }
    // 根据服务名与服务协议类型，获取服务端口号
    const char *serv_name = "daytime";
    struct servent *se = getservbyname(serv_name, "tcp");
    printf("port: %d\n", ntohs(se->s_port));
    return 0;
}