#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {

    if (argc != 2) {
        char *usage = "Usage: mycat2 <filename>\n";
        write(STDERR_FILENO, usage, strlen(usage));
        exit(EXIT_FAILURE);
    }


    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {

        page_size = 4096;
        fprintf(stderr, "Warning: Failed to get page size, using default %ld\n", page_size);
    }


    char *buffer = (char *)malloc(page_size);
    if (buffer == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }


    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    

    while ((bytes_read = read(fd, buffer, page_size)) > 0) {
        ssize_t bytes_written = 0;
        while (bytes_written < bytes_read) {
            ssize_t n = write(STDOUT_FILENO, buffer + bytes_written, bytes_read - bytes_written);
            if (n == -1) {
                perror("write failed");
                close(fd);
                free(buffer);
                exit(EXIT_FAILURE);
            }
            bytes_written += n;
        }
    }


    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1) {
        perror("close failed");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    free(buffer);

    return EXIT_SUCCESS;
}