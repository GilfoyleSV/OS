#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

void *library_handle = NULL;
int (*PrimeCount)(int, int);
float (*Pi)(int);

void load_library(const char* lib_name) {
    if (library_handle) {
        dlclose(library_handle);
    }

    library_handle = dlopen(lib_name, RTLD_LAZY);
    if (!library_handle) {
        fprintf(stderr, "Error loading library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    dlerror();

    PrimeCount = (int (*)(int,int)) dlsym(library_handle, "PrimeCount");
    Pi = (float (*)(int)) dlsym(library_handle, "Pi");

    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error finding symbol: %s\n", error);
        exit(EXIT_FAILURE);
    }

    printf("Switched to implementation: %s\n", lib_name);
}

int main() {
    const char* lib_names[] = {"./lib1.so", "./lib2.so"};
    int current_lib = 0;

    load_library(lib_names[current_lib]);

    printf("Program 2 (Runtime dynamic loading)\n");
    printf("Usage: \n 0 (Switch impl)\n 1 A B (PrimeCount)\n 2 K (Pi)\n");

    int cmd;
    while (scanf("%d", &cmd) != EOF) {
        if (cmd == 0) {
            current_lib = 1 - current_lib;
            load_library(lib_names[current_lib]);
        } 
        else if (cmd == 1) {
            int a, b;
            if (scanf("%d %d", &a, &b) == 2) {
                printf("Result: %d\n", PrimeCount(a, b));
            }
        } 
        else if (cmd == 2) {
            int k;
            if (scanf("%d", &k) == 1) {
                printf("Result: %f\n", Pi(k));
            }
        } 
        else {
            printf("Unknown command\n");
        }
    }

    if (library_handle) {
        if (dlclose(library_handle) != 0) {
            fprintf(stderr, "Error closing library: %s\n", dlerror());
        }
    }

    return 0;
}
