#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#define SHARED_SIZE 4096

int safe_close(int fd) {
    if (fd != -1 && close(fd) == -1) {
        perror("close failed");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <in_file> <out_file>\n", argv[0]);
        return 1;
    }

    const char *in_file = argv[1];  
    const char *out_file = argv[2];

    int fd_in = open(in_file, O_RDWR);
    int fd_out = open(out_file, O_RDWR);

    if (fd_in == -1 || fd_out == -1) {
        perror("child2: open failed");
        return 1;
    }

    char *mem_in = mmap(NULL, SHARED_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_in, 0);
    char *mem_out = mmap(NULL, SHARED_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_out, 0);

    safe_close(fd_in);
    safe_close(fd_out);

    if (mem_in == MAP_FAILED || mem_out == MAP_FAILED) {
        perror("child2: mmap failed");
        return 1;
    }

    printf("Child2 запущен. Ожидание данных...\n");
    
    while (1) {
        if (mem_in[0] != '\0') {
            
            size_t data_len = strnlen(mem_in, SHARED_SIZE - 1);
            
            int write_ind = 0;
            bool last_space = false;

            for (size_t read_ind = 0; read_ind < data_len; read_ind++) {
                char c = mem_in[read_ind];
                if (c == ' ') {
                    if (!last_space) {
                        mem_in[write_ind++] = c;
                        last_space = true;
                    }
                } else {
                    mem_in[write_ind++] = c;
                    last_space = false;
                }
            }

            mem_in[write_ind] = '\0';
            size_t to_write = strlen(mem_in);
            
            memcpy(mem_out, mem_in, to_write + 1);
            
            printf("Child2 обработал: %s", mem_out);
            mem_in[0] = '\0';
        }
        
        usleep(10000);
    }
    
    munmap(mem_in, SHARED_SIZE);
    munmap(mem_out, SHARED_SIZE);
    return 0;
}