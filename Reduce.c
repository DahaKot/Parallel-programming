#include <mpi.h>
#include <stdio.h>
#include <math.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    const int Data_size = 1;

    int N = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &N);

    if (N < 1) {
        printf("Try more processes\n");
        return -1;
    }

    int Rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &Rank);

    int buf = Rank+1;
    if (Rank == 0) {
        printf("REDUCE\nWtick is: %lg\n", MPI_Wtick());
    }

    double mean_time = 0;
    int n_loops = 1000000;

    double times[1000000];
    int buf0 = 0;

    for (int i = 0; i < n_loops; i++) {
        MPI_Barrier(MPI_COMM_WORLD);

        double start = MPI_Wtime();
        MPI_Reduce(&buf, &buf0, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
        double end = MPI_Wtime();

        mean_time += end - start;
        times[i] = end - start;
    }

    if (Rank == 0) {
        printf("buf is: %d\n", buf0);
    }

    mean_time /= n_loops;
    double std = 0;
    for (int i = 0; i < n_loops; i++) {
        std += (times[i] - mean_time) * (times[i] - mean_time);
    }

    std = sqrt(std / n_loops);
    
    printf("%d std: %lg\n", Rank, std);

    MPI_Barrier(MPI_COMM_WORLD);
    if (Rank == 0) {    
        printf("===========================\n");
    }

    MPI_Finalize();

    return 0;
}