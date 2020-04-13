#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

inline static int exit_with_errno(int ret_value) {
    if (ret_value < 0) {
      printf("%s\n", strerror(errno));
    }
    exit(ret_value);
}
