#define _GNU_SOURCE
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>

#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", basename(argv[1]));
        return -1;
    }

    int file_fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if (-1 == file_fd) {
        exit_with_errno(-1);
    }

    int file_pipe[2];
    int ret = pipe(file_pipe);
    if (-1 == ret) {
        exit_with_errno(-1);
    }

    int stdout_pipe[2];
    ret = pipe(stdout_pipe);
    if (-1 == ret) {
        exit_with_errno(-1);
    }
    ret = splice(STDIN_FILENO, NULL, stdout_pipe[1], NULL, 65536, SPLICE_F_MOVE|SPLICE_F_MORE);
    if (-1 == ret) {
        exit_with_errno(-1);
    }
    ret = tee(stdout_pipe[0], file_pipe[1], 65536, SPLICE_F_NONBLOCK);
    if (-1 == ret) {
        exit_with_errno(-1);
    }
    ret = splice(stdout_pipe[0], NULL, STDOUT_FILENO, NULL, 65536,  SPLICE_F_MOVE|SPLICE_F_MORE);
    if (-1 == ret) {
        exit_with_errno(-1);
    }
    ret = splice(file_pipe[0], NULL, file_fd, NULL, 65536, SPLICE_F_MOVE|SPLICE_F_MORE);

    return 0;
}