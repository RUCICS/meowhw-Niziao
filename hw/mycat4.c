#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <stdint.h>  // 添加这个头文件解决 uintptr_t 问题

char* align_alloc(size_t size) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) page_size = 4096;
    
    size_t total_size = size + page_size - 1;
    void* raw_ptr = malloc(total_size + sizeof(void*));
    if (raw_ptr == NULL) return NULL;
    
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

size_t get_optimal_buffer_size(int fd, size_t page_size) {
    struct statvfs fs_info;
    if (fstatvfs(fd, &fs_info) != 0) {
        perror("fstatvfs failed");
        return page_size;
    }

    unsigned long block_size = fs_info.f_bsize;
    if (block_size == 0) return page_size;
    
    size_t buffer_size = page_size > block_size ? page_size : block_size;
    if (buffer_size % block_size != 0) {
        buffer_size = ((buffer_size / block_size) + 1) * block_size;
    }
    
    size_t max_buffer = 4 * 1024 * 1024;
    while (buffer_size > max_buffer) {
        buffer_size /= 2;
        if (buffer_size < block_size) return block_size;
    }
    
    return buffer_size;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: mycat4 <filename>\n");
        exit(EXIT_FAILURE);
    }

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) page_size = 4096;

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = get_optimal_buffer_size(fd, page_size);
    char *buffer = align_alloc(buffer_size);
    if (buffer == NULL) {
        perror("align_alloc failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
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

    if (bytes_read == -1) perror("read failed");
    if (close(fd) == -1) perror("close failed");
    
    align_free(buffer);
    return bytes_read == -1 ? EXIT_FAILURE : EXIT_SUCCESS;
}
