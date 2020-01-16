#include <errno.h>
#include <string.h>
#include <stdio.h>

inline static int exit_with_errno(int ret_value) {
    printf("%s\n", strerror(errno));
    return ret_value;
}
