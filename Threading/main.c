#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <threads.h>
#include "unistd.h"

static const int NUM_THREADS = 100;

void* thread(void* argp) {
    int* i = (int*)argp;
    sleep(rand() % NUM_THREADS);
    printf("test %d\n", *i);
    pthread_exit(0);
}


int main() {

    pthread_t thread_id[NUM_THREADS];
    int args[NUM_THREADS];
    printf("before\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = i;
        pthread_create(&thread_id[i], NULL, thread, &args[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread_id[i], NULL);
    }
    printf("after\n");


    return 0;
}
