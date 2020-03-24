#include "functions.h"

int main(int argc, char *argv[]) {
    // common part with initialization
    MPI_Init(&argc, &argv);

    const int data_size = 1;

    int n = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &n);

    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double tick = MPI_Wtick();

    int root = 0;
    int root_buf[data_size * n]; // (n-1) * data_size is needed for gather
    int leaf_buf[data_size];

    if (rank == root) {
        printf("Wtick is: %lg\n", tick);
        for (int i = 0; i < data_size; i++) {
            root_buf[i] = i;
            leaf_buf[i] = 0;
        }
    }
    else {
        for (int i = 0; i < data_size; i++) {
            root_buf[i] = 0;
            leaf_buf[i] = n - rank + i;
        }
    }

    int n_loops = 1000000;
    double times[n_loops];
    int test = 0;

    // BCAST part
    double mean_time = 0;

    for (int i = 0; i < n_loops; i++) {
        MPI_Barrier(MPI_COMM_WORLD);

        double start = MPI_Wtime();
        Bcast(root_buf, data_size, MPI_INT, root, MPI_COMM_WORLD, rank, n);
        double end = MPI_Wtime();

        if (i == 0 && rank != root && test) {
            printf("BCAST\n");
            Print_Buf(root_buf, data_size, rank);
        }

        mean_time += end - start;
        times[i] = end - start;
    }

    mean_time /= n_loops;
    
    double std = Count_std(mean_time, times, n_loops);
    printf("BCAST %d std/tick: %lg\n", rank, std / tick);

    // REDUCE part
    mean_time = 0;

    for (int i = 0; i < n_loops; i++) {
        MPI_Barrier(MPI_COMM_WORLD);

        double start = MPI_Wtime();
        Reduce(leaf_buf, root_buf, data_size, MPI_INT, MPI_SUM, 
                root, MPI_COMM_WORLD, rank, n);
        double end = MPI_Wtime();

        if (i == 0 && rank == root && test) {
            printf("REDUCE\n");
            Print_Buf(root_buf, data_size, rank);
        }

        mean_time += end - start;
        times[i] = end - start;
    }

    mean_time /= n_loops;
    
    std = Count_std(mean_time, times, n_loops);
    printf("REDUCE %d std/tick: %lg\n", rank, std / tick);

    // GATHER part
    mean_time = 0;

    for (int i = 0; i < n_loops; i++) {
        MPI_Barrier(MPI_COMM_WORLD);

        double start = MPI_Wtime();
        Gather(leaf_buf, data_size, MPI_INT, root_buf, data_size, 
                MPI_INT, root, MPI_COMM_WORLD, rank, n);
        double end = MPI_Wtime();

        if (i == 0 && rank == root && test) {
            printf("GATHER\n");
            Print_Buf(root_buf, data_size * n, rank);
        }

        mean_time += end - start;
        times[i] = end - start;
    }

    mean_time /= n_loops;
    
    std = Count_std(mean_time, times, n_loops);
    printf("GATHER %d std/tick: %lg\n", rank, std / tick);

    // SCATTER part
    mean_time = 0;

    for (int i = 0; i < data_size; i++) {
        leaf_buf[i] = 0;
    }

    for (int i = 0; i < n_loops; i++) {
        MPI_Barrier(MPI_COMM_WORLD);

        double start = MPI_Wtime();
        Scatter(root_buf, data_size, MPI_INT, leaf_buf, data_size, 
                MPI_INT, root, MPI_COMM_WORLD, rank, n);
        double end = MPI_Wtime();

        if (i == 0 && rank != root && test) {
            printf("SCATTER\n");
            Print_Buf(leaf_buf, data_size, rank);
        }

        mean_time += end - start;
        times[i] = end - start;
    }

    mean_time /= n_loops;
    
    std = Count_std(mean_time, times, n_loops);
    printf("SCATTER %d std/tick: %lg\n", rank, std / tick);

    MPI_Finalize();

    return 0;
}