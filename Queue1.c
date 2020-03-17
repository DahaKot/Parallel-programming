#include <stdio.h>
#include <mpi.h>

int main (int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int N = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &N);

    if (N < 1) {
        printf("Try more processes\n");
        return -1;
    }

    int Rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &Rank);

    MPI_Status Status;
    char Buf = 0;

    if (Rank == 0) {
        printf("%d\n", Rank);

        for (int i = 1; i < N; i++) {
            MPI_Send(&Buf, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);

            MPI_Recv(&Buf, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, &Status);
        }
    }
    else {
        MPI_Recv(&Buf, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &Status);

        printf("%d\n", Rank);
        fflush(stdout);

        MPI_Send(&Buf, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
}