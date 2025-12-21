#include "lib.h"
#include <math.h>
#include <stdio.h>

int isPrimeNaive(int n) {
    if (n < 2) return 0;
    for (int i = 2; i < n; ++i) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int PrimeCount(int A, int B) {
    int count = 0;
    for (int i = A; i <= B; ++i) {
        if (isPrimeNaive(i)) {
            count++;
        }
    }
    return count;
}

// ряд лейбница
float Pi(int K) {
    float sum = 0.0;
    for (int i = 0; i < K; ++i) {
        float term = 1.0 / (2 * i + 1);
        if (i % 2 == 1) {
            sum -= term;
        } else {
            sum += term;
        }
    }
    return 4.0 * sum;
}