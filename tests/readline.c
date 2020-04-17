#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "../socket-api/utils.h"

// 对比一下getline函数的行为，但不像getline那样会重新分配空间
int32_t read_line(int fd, char *buf, int32_t max_line_size) {
  int32_t n_read = 0;
  while (n_read < max_line_size - 1) {
    char c;
    int32_t n = read(fd, &c, 1);
    if (n == 0) {
      break;
    }
    if (n == -1) {
      return -1;
    }
    // 会将换行符读入，并计算字符数
    buf[n_read++] = c;
    if (c == '\n') {
      break;
    }
  }
  // 在最后一位上添加字符串结束标志
  buf[n_read] = '\0';

  return n_read;
}

// 程序从标准输入中循环不断的读入数据，每次读入一行，然后打印出来
//
int main(int argc, char *argv[]) {
  const int32_t kMaxLineLen = 1024;
  char line_text[kMaxLineLen];
  while (1) {
    int32_t n_read = read_line(STDIN_FILENO, line_text, kMaxLineLen);
    if (n_read < 0) {
      exit_with_errno(n_read);
    }
    if (n_read == 0) {
      printf("EOF!\n");
      break;
    }
    printf("%s", line_text);
  }
  return 0;
}