#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
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
        perror("child1: open failed");
        return 1;
    }

    char *mem_in = mmap(NULL, SHARED_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_in, 0);
    char *mem_out = mmap(NULL, SHARED_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_out, 0);

    safe_close(fd_in);
    safe_close(fd_out);

    if (mem_in == MAP_FAILED || mem_out == MAP_FAILED) {
        perror("child1: mmap failed");
        return 1;
    }

    printf("Child1 запущен. Ожидание данных...\n");

    
    while (1) {

        if (mem_in[0] != '\0') { 
            
            size_t data_len = strnlen(mem_in, SHARED_SIZE - 1);
            

            for (size_t i = 0; i < data_len; i++) {
                mem_in[i] = toupper((unsigned char)mem_in[i]);
            }
            

            memcpy(mem_out, mem_in, data_len);
            mem_out[data_len] = '\0';
            
            printf("Child1 обработал: %s", mem_out);


            mem_in[0] = '\0'; 
        }
        
        if (feof(stdin) && mem_in[0] == '\0') {
            break;
        }

        usleep(10000);
    }
    
    munmap(mem_in, SHARED_SIZE);
    munmap(mem_out, SHARED_SIZE);
    return 0;
}