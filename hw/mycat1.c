#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        char *usage = "Usage: mycat1 <filename>\n";
        write(STDERR_FILENO, usage, strlen(usage));
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    char c;
    ssize_t bytes_read;

    while ((bytes_read = read(fd, &c, 1)) > 0) {
        ssize_t bytes_written = write(STDOUT_FILENO, &c, 1);
        if (bytes_written == -1) {
            perror("write failed");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }


    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        exit(EXIT_FAILURE);
    }


    if (close(fd) == -1) {
        perror("close failed");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}