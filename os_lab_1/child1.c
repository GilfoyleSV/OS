#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int main(void) {
    char *buffer = NULL;
    size_t buffer_size = 0;
    ssize_t read_size;

    while ((read_size = getline(&buffer, &buffer_size, stdin)) != -1) {
        for (ssize_t i = 0; i < read_size; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }

        if (write(STDOUT_FILENO, buffer, read_size) == -1) {
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
