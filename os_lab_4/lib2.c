#include "lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int PrimeCount(int A, int B) {
    if (B < 2) return 0;
    
    char *isPrime = (char*)malloc((B + 1) * sizeof(char));
    
    memset(isPrime, 1, (B + 1) * sizeof(char));
    
    isPrime[0] = 0;
    isPrime[1] = 0;
    
    for (int i = 2; i * i <= B; ++i) {
        if (isPrime[i]) {
            for (int j = i * i; j <= B; j += i) {
                isPrime[j] = 0;
            }
        }
    }
    
    int count = 0;
    for (int i = A; i <= B; ++i) {
        if (isPrime[i]) {
            count++;
        }
    }
    
    free(isPrime);
    return count;
}
// формула валлиса
float Pi(int K) {
    float product = 1.0;
    for (int i = 1; i <= K; ++i) {
        float numerator = 4.0 * i * i;
        float denominator = 4.0 * i * i - 1;
        product *= (numerator / denominator);
    }
    return product * 2.0;
}