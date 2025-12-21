#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>

typedef struct {
    int *array;
    int start;
    int end;
    int local_min;
    int local_max;
} ThreadData;

int max_threads;

void find_minmax(void *arg) {
    ThreadData *data = (ThreadData*)arg;
    int min = INT_MAX;
    int max = INT_MIN;

    for (long i = data->start; i < data->end; i++) {
        if (data->array[i] < min) min = data->array[i];
        if (data->array[i] > max) max = data->array[i];
    }

    data->local_min = min;
    data->local_max = max;

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Использование: %s <макс_число_потоков>\n", argv[0]);
        return 1;
    }

    max_threads = atoi(argv[1]);
    if (max_threads < 1) {
        printf("Недопустимое значение: %d\n", max_threads);
        exit(-1);
    }

    const int ARRAY_SIZE = 10000000;

    int *array = malloc(ARRAY_SIZE * sizeof(int));
    if (!array) {
        perror("Ошибка выделения памяти");
        exit(-1);
    }

    for (long i = 0; i < ARRAY_SIZE; i++)
        array[i] = rand();

    int num_chunks = max_threads;
    int chunk_size = ARRAY_SIZE / num_chunks;

    pthread_t threads[max_threads];
    ThreadData thread_data[max_threads];

    int global_min = INT_MAX;
    int global_max = INT_MIN;

    for (int i = 0; i < num_chunks; i++) {
        ThreadData element;
        element.array = array;
        element.start = i * chunk_size;
        if (i == num_chunks - 1)
            element.end = ARRAY_SIZE;
        else
            element.end = (i + 1) * chunk_size;

        thread_data[i] = element;

        if (pthread_create(&threads[i], NULL, find_minmax, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            exit(-1);
        }
    }

    for (int i = 0; i < max_threads; i++){
        pthread_join(threads[i], NULL);
    }



    for (int i = 0; i < max_threads; i++) {
        if (thread_data[i].local_min < global_min)
            global_min = thread_data[i].local_min;
        if (thread_data[i].local_max > global_max)
            global_max = thread_data[i].local_max;
    }

    printf("Минимум: %d\n", global_min);
    printf("Максимум: %d\n", global_max);

    free(array);
    return 0;
}
