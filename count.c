#include <stdlib.h>

#include <mpi.h>
#include <stdio.h>
#include <math.h>

double count_result(int n);

int main (int argc, char **argv) {
    // common part with initialization
    MPI_Init(&argc, &argv);

    const int data_size = 1;

    int n = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &n);

    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int len = atoi(argv[1]);

    MPI_Status Status;
    double *results = (double *) calloc(len, sizeof(double));

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    if (rank == 0) {
        // fill own values
        for (int i = 1; i <= len; i += n) {
            results[i - 1] = count_result(i);
        }

        // wait for others to count
        for (int i = 1; i <= len; i++) {
            if (i % n != 1) {
                MPI_Recv(results + i - 1, 1, MPI_DOUBLE, (i - 1) % n, i, MPI_COMM_WORLD, &Status);
            }
        }

        // sum everything up
        double end_result = 0;
        for (int i = len - 1; i >= 0; i--) {
            end_result += results[i];
        }
        printf("\n%.20lg\n", end_result);

        double end_time = MPI_Wtime();
        printf("%d time is: %lg\n", rank, end_time - start_time);
    }
    else {
        for (int i = rank + 1; i <= len; i += n) {
            double result = count_result(i);
            MPI_Send(&result, 1, MPI_DOUBLE, 0, i, MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    free(results);

    return 0;
}

double count_result(int n) {
    return (double) 6 / (n * n * M_PI * M_PI);
}