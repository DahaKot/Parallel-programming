#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int N = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &N);

    if (N < 1) {
        printf("Try more processes\n");
        return -1;
    }

    int Rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &Rank);

    int buf[100];
    if (Rank == 0) {
        for (int i = 0; i < 100; i++) {
            buf[i] = i;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double start = MPI_Wtime();

    MPI_Bcast(buf, 100, MPI_INT, 0, MPI_COMM_WORLD);

    printf("%d Broadcast time: %lg\n", Rank, MPI_Wtime() - start);

    MPI_Finalize();

    return 0;
}