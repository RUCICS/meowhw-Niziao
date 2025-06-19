#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>  // 添加这个头文件

char* align_alloc(size_t size) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        page_size = 4096;
    }

    size_t total_size = size + page_size - 1;
    void* raw_ptr = malloc(total_size + sizeof(void*));
    if (raw_ptr == NULL) {
        return NULL;
    }
    
    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = (raw_addr + page_size - 1) & ~(page_size - 1);
    *((void**)(aligned_addr - sizeof(void*))) = raw_ptr;
    
    return (char*)aligned_addr;
}

void align_free(void* ptr) {
    if (ptr == NULL) return;
    void* raw_ptr = *((void**)((uintptr_t)ptr - sizeof(void*)));
    free(raw_ptr);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: mvcat3 <filename>\n");
        exit(EXIT_FAILURE);
    }

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        page_size = 4096;
        fprintf(stderr, "Warning: Failed to get page size, using default %ld\n", page_size);
    }

    char *buffer = align_alloc(page_size);
    if (buffer == NULL) {
        perror("align_alloc failed");
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        align_free(buffer);
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
                align_free(buffer);
                exit(EXIT_FAILURE);
            }
            bytes_written += n;
        }
    }

    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        align_free(buffer);
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1) {
        perror("close failed");
        align_free(buffer);
        exit(EXIT_FAILURE);
    }

    align_free(buffer);
    return EXIT_SUCCESS;
}
