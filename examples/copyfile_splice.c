#define _GNU_SOURCE
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>

#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <src> <dst>\n", basename(argv[0]));
        return -1;
    }
    int in_file = open(argv[1], O_RDONLY);
    int out_file = open(argv[2], O_RDWR | O_CREAT, 0664);
    if (-1 == in_file || -1 == out_file) {
        exit_with_errno(-1);
    }

    int pipe_fd[2];
    int ret = pipe(pipe_fd);
    if (ret != 0) {
        exit_with_errno(-1);
    }
    // 求取文件的大小
    int file_size = lseek(in_file, 0, SEEK_END);
    // 将文件指针重置回文件开始
    lseek(in_file, 0, SEEK_SET);

    // 通过pipe进行数据搬运，最大一次只能搬运65536，所以如果文件太大，要分多次搬运
    const int max_move_size = 65536;
    int remain_data_size = file_size;
    while (remain_data_size > 0) {
        // 将源文件内容写入到通道
        int n_writen = splice(in_file, 0, pipe_fd[1], NULL, max_move_size, SPLICE_F_MOVE);
        // 将通道中的内容写入到目标文件中
        int n_read = splice(pipe_fd[0], NULL, out_file, NULL, n_writen, SPLICE_F_MOVE);
        remain_data_size -= n_writen;
    }
    
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    close(in_file);
    close(out_file);
}