#include <stdio.h>
#include <mpi.h>

// plus idea: broadcast a message with tag!

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

    char Buf = 0;
    MPI_Status Status;

    MPI_Recv(&Buf, 1, MPI_CHAR, Rank - 1, 0, MPI_COMM_WORLD, &Status);

    printf("%d\n", Rank);
    fflush(stdout);

    if (Rank < N - 1) {
        MPI_Send(&Buf, 1, MPI_CHAR, Rank + 1, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
}