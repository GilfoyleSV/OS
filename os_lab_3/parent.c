#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#define SHARED_SIZE 4096

#define MAP_FILE_1 "mmap_pipe_1.tmp"
#define MAP_FILE_MID "mmap_pipe_mid.tmp"
#define MAP_FILE_2 "mmap_pipe_2.tmp"

int safe_close(int fd) {
    if (fd != -1 && close(fd) == -1) {
        perror("close failed");
        return -1;
    }
    return 0;
}

pid_t create_process() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    return pid;
}

char* setup_mmap(const char *filename, int *fd_out) {
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open mmap file failed");
        return NULL;
    }

    if (ftruncate(fd, SHARED_SIZE) == -1) {
        perror("ftruncate failed");
        safe_close(fd);
        return NULL;
    }

    char *mem = mmap(NULL, SHARED_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mmap failed");
        safe_close(fd);
        return NULL;
    }

    *fd_out = fd;
    return mem;
}

void cleanup_and_exit(char *mem1, int fd1, char *mem_mid, int fd_mid, char *mem2, int fd2, int status) {
    if (mem1 != MAP_FAILED) {
        if (munmap(mem1, SHARED_SIZE) == -1) {
            perror("munmap mem1 failed");
        }
    }
    if (mem_mid != MAP_FAILED) {
        if (munmap(mem_mid, SHARED_SIZE) == -1) {
            perror("munmap mem_mid failed");
        }
    }
    if (mem2 != MAP_FAILED) {
        if (munmap(mem2, SHARED_SIZE) == -1) {
            perror("munmap mem2 failed");
        }
    }

    safe_close(fd1);
    safe_close(fd_mid);
    safe_close(fd2);

    if (unlink(MAP_FILE_1) == -1) perror("unlink MAP_FILE_1 failed");
    if (unlink(MAP_FILE_MID) == -1) perror("unlink MAP_FILE_MID failed");
    if (unlink(MAP_FILE_2) == -1) perror("unlink MAP_FILE_2 failed");

    exit(status);
}


int main() {
    int fd1 = -1, fd_mid = -1, fd2 = -1;
    char *mem1 = MAP_FAILED, *mem_mid = MAP_FAILED, *mem2 = MAP_FAILED;

    mem1 = setup_mmap(MAP_FILE_1, &fd1);
    mem_mid = setup_mmap(MAP_FILE_MID, &fd_mid);
    mem2 = setup_mmap(MAP_FILE_2, &fd2);

    if (mem1 == MAP_FAILED || mem_mid == MAP_FAILED || mem2 == MAP_FAILED) {
        cleanup_and_exit(mem1, fd1, mem_mid, fd_mid, mem2, fd2, 1);
    }
    
    char fd1_str[10], fd_mid_str[10], fd2_str[10];

    pid_t pid1 = create_process();
    if (pid1 == 0) {
        safe_close(fd_mid);
        safe_close(fd2); 

        execl("./child1", "child1", MAP_FILE_1, MAP_FILE_MID, NULL);
        perror("execl child1 failed");
        cleanup_and_exit(mem1, fd1, mem_mid, fd_mid, mem2, fd2, 1);
    }

    pid_t pid2 = create_process();
    if (pid2 == 0) {
        safe_close(fd1);
        
        execl("./child2", "child2", MAP_FILE_MID, MAP_FILE_2, NULL);
        perror("execl child2 failed");
        cleanup_and_exit(mem1, fd1, mem_mid, fd_mid, mem2, fd2, 1);
    }

    safe_close(fd_mid);

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    
    char write_buf[SHARED_SIZE];
    char read_buf[SHARED_SIZE];

    printf("Введите строку:\n");
    while ((nread = getline(&line, &len, stdin)) != -1) {
        if (nread > SHARED_SIZE - 1) nread = SHARED_SIZE - 1;
        memcpy(mem1, line, nread);
        mem1[nread] = '\0';
        size_t r = strlen(mem2);
        if (r > 0) {
            printf("Result: %s", mem2);
            mem2[0] = '\0'; 
        } else {
            printf("Result: (empty or not ready)\n");
        }
    }

    free(line);
    
    printf("\nОжидание завершения дочерних процессов...\n");

    int status;
    if (waitpid(pid1, &status, 0) == -1){
        perror("waitpid child1 failed");
    }
    if (WIFEXITED(status)) {
        printf("Child1 завершился с кодом %d\n", WEXITSTATUS(status));
    }
    
    if (waitpid(pid2, &status, 0) == -1){
        perror("waitpid child2 failed");
    }
    if (WIFEXITED(status)) {
        printf("Child2 завершился с кодом %d\n", WEXITSTATUS(status));
    }

    cleanup_and_exit(mem1, fd1, mem_mid, fd_mid, mem2, fd2, 0);
    return 0;
}