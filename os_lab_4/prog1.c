#include <stdio.h>
#include "lib.h"

int main() {
    int cmd;
    printf("Program 1 (Compile-time linking with lib1)\n");
    printf("Usage: \n 1 A B (PrimeCount)\n 2 K (Pi)\n\n");

    while (scanf("%d", &cmd) != EOF) {
        if (cmd == 1) {
            int a, b;
            if (scanf("%d %d", &a, &b) == 2) {
                printf("PrimeCount(%d, %d) = %d\n", a, b, PrimeCount(a, b));
            }
        } else if (cmd == 2) {
            int k;
            if (scanf("%d", &k) == 1) {
                printf("Pi(%d) = %f\n", k, Pi(k));
            }
        } else {
            printf("Unknown command for Prog1\n");
        }
    }
    return 0;
}