#include <stdlib.h>

#include <mpi.h>
#include <stdio.h>
#include <math.h>

struct information {
    int start;
    int end;
};

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

    struct information info = {0, 0};

    MPI_Status Status;

    if (n <= len) {
        info.start = rank * (len/n) + 1;
        info.end   = rank * (len/n) + len / n;
        if (rank == n - 1) {
            info.end = len;
        }
    }
    else {
        info.start = rank + 1;
        info.end   = rank + 2;
    }

    double *results = (double *) calloc(len, sizeof(double));

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    if (rank == 0) {
        for (int i = 1; i <= info.end; i++) {
            results[i - 1] = count_result(i);
        }

        for (int i = 1; i < n; i++) {
            struct information process_info;
            MPI_Recv(&process_info, 2, MPI_INT, i, 0, MPI_COMM_WORLD, &Status);

            int msg_len = process_info.end - process_info.start + 1;

            for (int j = process_info.start; j <= process_info.end && j <= len; j++) {
                MPI_Recv(results + j - 1, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &Status);
            }
        }

        double end_result = 0;
        for (int i = len - 1; i >= 0; i--) {
            end_result += results[i];
        }
        printf("%.20lg\n", end_result);
    }
    else {
        MPI_Send(&info, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);

        for (int i = info.start; i <= info.end; i++) {
            double result = count_result(i);
            MPI_Send(&result, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
        }
    }

    double end_time = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);

    printf("%d time is: %lg\n", rank, end_time - start_time);

    MPI_Barrier(MPI_COMM_WORLD);

    free(results);

    return 0;
}

double count_result(int n) {
    return (double) 6 / (n * n * M_PI * M_PI);
}