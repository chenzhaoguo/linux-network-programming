#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "utils.h"

int main(int argc, char *argv[]) {
    uid_t uid = getuid();
    uid_t euid = geteuid();

    printf("userid is %d, effective userid is: %d\n", uid, euid);

    int ret = open("/etc/passwd", O_RDWR);
    if (0 != ret) {
        exit_with_errno(-1);
    }
    return 0;
}