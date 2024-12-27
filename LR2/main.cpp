#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>



int **arrays; // Массив указателей на указатели
int *result;
int num_arrays;
int array_length;
int max_threads;
pthread_mutex_t mutex;
//cuncorense 


void* sum_subarrays(void *arg) {  //когда длина больше количества
    int index = *(int *)arg;
    free(arg);

    int start = index * (array_length / max_threads);
    int end = (index + 1) * (array_length / max_threads);
    if (index == max_threads - 1) {
        end = array_length;
    }

    for (int j = 0; j < num_arrays; j++) {
        for (int i = start; i < end; i++) { 
            result[i] += arrays[j][i]; 
        }
    }
    pthread_exit(0);
}


void* sum_arrays(void *arg) {  //когда количество больше длины
    int index = *(int *)arg;
    free(arg);
    int *tmp = (int *)calloc(array_length, sizeof(int));

    int start = index * (num_arrays / max_threads);
    int end = (index + 1) * (num_arrays / max_threads);
    if (index == max_threads - 1) {
        end = num_arrays;
    }

    for (int j = start; j < end; j++) {
        for (int i = 0; i < array_length; i++) { 
            tmp[i] += arrays[j][i]; 
        }
    }

    for (int i = 0; i < array_length; i++) {
        pthread_mutex_lock(&mutex); 
        result[i] += tmp[i]; 
        pthread_mutex_unlock(&mutex); 
    }

    free(tmp);
    pthread_exit(0);
}


int main(int argc, char* argv[]) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    if (argc < 4) {
        printf("Usage: %s <num_arrays> <array_length> <max_threads>\n", argv[0]);
        return -1;
    }

    num_arrays = atoi(argv[1]);
    array_length = atoi(argv[2]);
    max_threads = atoi(argv[3]);

    if (num_arrays <= 0 || array_length <= 0 || max_threads <= 0) {
        printf("All input values must be positive integers.\n");
        return -1;
    }

    arrays = (int **)malloc(num_arrays * sizeof(int *));
    for (int i = 0; i < num_arrays; i++) {
        arrays[i] = (int *)malloc(array_length * sizeof(int));
        for (int j = 0; j < array_length; j++) {
            arrays[i][j] = 1; // пусть будет 1 для примера
        }
    }
    result = (int *)calloc(array_length, sizeof(int));

    pthread_mutex_init(&mutex, NULL);
    pthread_t tid[max_threads];

    if (array_length >= num_arrays) {

        for (int i = 0; i < max_threads; i ++) {
            int *index = (int *)malloc(sizeof(int));
            *index = i;
            if (pthread_create(&tid[i], NULL, sum_subarrays, index) != 0) {
                perror("Failed to create thread");
                return -1;
            }
        }
    } else {

        for (int i = 0; i < max_threads; i ++) {
            int *index = (int *)malloc(sizeof(int));
            *index = i;
            if (pthread_create(&tid[i], NULL, sum_arrays, index) != 0) {
                perror("Failed to create thread");
                return -1;
            }
        }
    }

    for (int i = 0; i < max_threads; i++) {
        pthread_join(tid[i], NULL);
    }  

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double total_time = (end_time.tv_sec - start_time.tv_sec) + 
                        (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Time taken: %f seconds\n", total_time);
    printf("program done!\n");

    for (int i = 0; i < num_arrays; i++) {
        free(arrays[i]);
    }
    free(arrays);
    free(result);
    pthread_mutex_destroy(&mutex);

    return 0;
}