#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

#include "mpi.h"

#define COMPARE_TEST 0

#define DATA_COUNT 80000
#define ROOT 0

#define MERGETAG 1
#define SORTEDTAG 2

#define ARRAY_SIZE(x) sizeof((x)) / (sizeof((x)[0]))

void show_array(int *array, size_t size) {
    for (size_t i = 0; i < size; i++) {
        printf("%d, ", array[i]);
    }
    printf("\n");
}

int cmpfunc (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

int bubble_sort(int *array, size_t size) {
    int tmp;

    for (int i = size - 1; i > 0; i--) {
        for (int j = 0; j < i; j++) {
            if (array[j] > array[j + 1]) {
                tmp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = tmp;
            }
        }
    }
}

void merge(int *array, int *remote, int *all, size_t size) {
    size_t all_count = 0;
    size_t j = 0;

    for (size_t i = 0; i < size; i++) {
        for (; remote[j] < array[i] && j < size; j++) {
            all[all_count++] = remote[j];
        }
        all[all_count++] = array[i];        
    }
    for(;j < size; j++) {
        all[all_count++] = remote[j];
    }
}

void sort(int *array, size_t size, int rank, int p) {
    int even_partner = rank;
    int odd_partner = rank;
    int remote[size];
    int all[size * 2];
    MPI_Status status;

    if (rank % 2) { /* odd rank */
        even_partner--;
        if (++odd_partner == p) {
            odd_partner = -1;
        }
    } else { /* even rank */
        if (++even_partner == p) {
            even_partner = -1;
        }
        odd_partner--;
    }
    // printf("Rank: %d, odd partner: %d, even partner: %d\n", rank, odd_partner, even_partner);
    bubble_sort(array, size);

    for (int phase = 0; phase < p; phase++) {
        int partner = phase % 2 ? odd_partner : even_partner; 
        if (partner >= 0) {
            if (rank < partner) {
                MPI_Send(array, size, MPI_INT, partner, MERGETAG, MPI_COMM_WORLD);
                MPI_Recv(array, size, MPI_INT, partner, SORTEDTAG, MPI_COMM_WORLD, &status);
            } else {
                MPI_Recv(remote, size, MPI_INT, partner, MERGETAG, MPI_COMM_WORLD, &status);
                merge(array, remote, all, size);
                for (size_t i = 0; i < size; i++) {
                    array[i] = all[i + size];
                }
                MPI_Send(all, size, MPI_INT, partner, SORTEDTAG, MPI_COMM_WORLD);
            }
        }
    }
}

int main(int argc, char** argv) {
    int my_rank, p;
    int global_array[DATA_COUNT];
    double loc_elapsed, elapsed;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);


    
    srand(time(0));
    for (uint32_t i = 0; i < sizeof(global_array) / sizeof(int); i++) {
        global_array[i] = rand() % 10000;
    }
    if (my_rank == ROOT) show_array(global_array, ARRAY_SIZE(global_array));
    
    int array[DATA_COUNT / p];
    clock_t start = clock();
#if !COMPARE_TEST
    MPI_Scatter(global_array, DATA_COUNT / p, MPI_INT, array, ARRAY_SIZE(array), MPI_INT, ROOT, MPI_COMM_WORLD);
    sort(array, ARRAY_SIZE(array), my_rank, p);
    MPI_Gather(array, ARRAY_SIZE(array), MPI_INT, global_array, ARRAY_SIZE(array), MPI_INT, ROOT, MPI_COMM_WORLD);
#else
    bubble_sort(global_array, ARRAY_SIZE(global_array));
#endif
    loc_elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    MPI_Reduce(&loc_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, ROOT, MPI_COMM_WORLD);
    if (my_rank == ROOT) {
        show_array(global_array, ARRAY_SIZE(global_array));
        printf("\nElapsed time: %fs\n", elapsed);
    }

    MPI_Finalize();
    return 0;
}
