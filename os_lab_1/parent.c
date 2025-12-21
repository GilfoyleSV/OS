#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <string.h>

int safe_close(int fd) {
    if (close(fd) == -1) {
        return -1;
    }
    return 0;
}

int create_process() {
    int pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    return pid;
}

int main() {
    int pipe1[2], pipe2[2], pipe_mid[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe_mid) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid1 = create_process();
    if (pid1 == 0) {
        
        safe_close(pipe1[1]);
        safe_close(pipe2[0]);
        safe_close(pipe2[1]);
        safe_close(pipe_mid[0]);

        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("dup2 pipe1[0]");
            exit(1);
        }
        if (dup2(pipe_mid[1], STDOUT_FILENO) == -1) {
            perror("dup2 pipe_mid[1]");
            exit(1);
        }

        safe_close(pipe1[0]);
        safe_close(pipe_mid[1]);

        execl("./child1", "child1", NULL);
        perror("execl child1 failed");
        exit(1);
    }

    pid_t pid2 = create_process();
    if (pid2 == 0) {
        safe_close(pipe1[0]);
        safe_close(pipe1[1]);
        safe_close(pipe_mid[1]);
        safe_close(pipe2[0]);

        if (dup2(pipe_mid[0], STDIN_FILENO) == -1) {
            perror("dup2 pipe_mid[0]");
            exit(1);
        }
        if (dup2(pipe2[1], STDOUT_FILENO) == -1) {
            perror("dup2 pipe2[1]");
            exit(1);
        }

        safe_close(pipe_mid[0]);
        safe_close(pipe2[1]);

        execl("./child2", "child2", NULL);
        perror("execl child2 failed");
        exit(1);
    }


    safe_close(pipe1[0]);
    safe_close(pipe_mid[0]);
    safe_close(pipe_mid[1]);
    safe_close(pipe2[1]);

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    char buf[1024];

    while ((nread = getline(&line, &len, stdin)) != -1) {
        if (write(pipe1[1], line, nread) == -1) {
            perror("write to pipe1 failed");
            break;
        }

        ssize_t r = read(pipe2[0], buf, sizeof(buf) - 1);
        if (r == -1) {
            perror("read from pipe2 failed");
            break;
        }
        if (r > 0) {
            buf[r] = '\0';
            printf("Result: %s", buf);
        }
    }

    free(line);
    safe_close(pipe1[1]);
    safe_close(pipe2[0]);

    if (waitpid(pid1, NULL, 0) == -1){
        perror("waitpid child1 failed");
    }
        
    if (waitpid(pid2, NULL, 0) == -1){
        perror("waitpid child2 failed");
    }
        
    return 0;
}
