#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

int main(void) {
    char *buffer = NULL;
    size_t buffer_size = 0;
    ssize_t read_size;

    while ((read_size = getline(&buffer, &buffer_size, stdin)) != -1) {
        int write_ind = 0;
        bool last_space = false;

        for (ssize_t read_ind = 0; read_ind < read_size; read_ind++) {
            char c = buffer[read_ind];
            if (c == ' ') {
                if (!last_space) {
                    buffer[write_ind++] = c;
                    last_space = true;
                }
            } else {
                buffer[write_ind++] = c;
                last_space = false;
            }
        }

        buffer[write_ind] = '\0';
        ssize_t to_write = strlen(buffer);

        if (write(STDOUT_FILENO, buffer, to_write) == -1) {
            perror("write failed");
            free(buffer);
            return 1;
        }
    }

    if (ferror(stdin)) {
        perror("getline failed");
        free(buffer);
        return 1;
    }

    free(buffer);

    return 0;
}
