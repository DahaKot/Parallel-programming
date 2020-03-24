#include "functions.h"

void Print_Buf(int *buf, int len, int rank) {
    for (int i = 0; i < len; i++) {
        printf("%d %d\n", rank, buf[i]);
    }
}

double Count_std(double mean_time, double *times, int n_loops) {
    double std = 0;
    for (int i = 0; i < n_loops; i++) {
        std += (times[i] - mean_time) * (times[i] - mean_time);
    }

    std = sqrt(std / n_loops);
    return std;
}

int Bcast(void *buf, int count, MPI_Datatype type, int root, 
            MPI_Comm comm, int rank, int n) {
    MPI_Status status;
    if (rank == root) {
        for (int i = 0; i < n; i++) {
            if (i != root) {
                MPI_Send(buf, count, type, i, 0, MPI_COMM_WORLD);
            }
        }
    }
    else {
        MPI_Recv(buf, count, type, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    }
    
    return 0;
}

int Apply(MPI_Op op, void *buf_, int count, int *loc) {
    int result = 0;
    int *buf = buf_;

    if (op == MPI_MAX) {
        result = -1000000;
        for (int i = 0; i < count; i++) {
            if (buf[i] > result) {
                result = buf[i];
            }
        } 
    }
    else if (op == MPI_MIN) {
        result = 1000000;
        for (int i = 0; i < count; i++) {
            if (buf[i] < result) {
                result = buf[i];
            }
        } 
    }
    else if (op == MPI_SUM) {
        for (int i = 0; i < count; i++) {
            result += buf[i];
        } 
    }
    else if (op == MPI_PROD) {
        result = 1;
        for (int i = 0; i < count; i++) {
            result *= buf[i];
        } 
    }
    else if (op == MPI_BAND) {
        result = 0xfffffff;
        for (int i = 0; i < count; i++) {
            result &= buf[i];
        } 
    }
    else if (op == MPI_LAND) {
        result = 1;
        for (int i = 0; i < count; i++) {
            result = result && buf[i];
        } 
    }
    else if (op == MPI_BOR) {
        for (int i = 0; i < count; i++) {
            result |= buf[i];
        } 
    }
    else if (op == MPI_LOR) {
        for (int i = 0; i < count; i++) {
            result = result || buf[i];
        } 
    }
    else if (op == MPI_BXOR) {
        result = 0xfffffff;
        for (int i = 0; i < count; i++) {
            result ^= buf[i];
        } 
    }
    else if (op == MPI_LXOR) {
        result = 1;
        for (int i = 0; i < count; i++) {
            result = result != buf[i];
        } 
    }
    else if (op == MPI_MAXLOC) {
        result = -1000000;
        for (int i = 0; i < count; i++) {
            if (buf[i] > result) {
                result = buf[i];
                *loc = i;
            }
        } 
    }
    else if (op == MPI_MINLOC) {
        result = 1000000;
        for (int i = 0; i < count; i++) {
            if (buf[i] < result) {
                result = buf[i];
                *loc = i;
            }
        } 
    }

    return result;
}

int Reduce(void *sbuf_, void *rbuf_, int count, MPI_Datatype type, MPI_Op op, 
            int root, MPI_Comm comm, int rank, int n) {
    MPI_Status status;
    int *rbuf = rbuf_;
    int *sbuf = sbuf_;
    int loc = -1;

    if (rank == root) {
        for (int i = 0; i < count; i++) {
            int temp_buf[100];
            for (int j = 0; j < n; j++) {
                if (j != root) {
                    MPI_Recv(temp_buf + j, 1, type, j, i, MPI_COMM_WORLD, &status);
                }
                else {
                    temp_buf[j] = sbuf[i];
                }
            }

            int result = Apply(op, temp_buf, n, &loc);

            if (op == MPI_MINLOC || op == MPI_MAXLOC) {
                if (loc == -1) {
                    printf("max/min mot found\n");
                    return 1;
                }

                rbuf[2 * i] = result;
                rbuf[2 * i + 1] = loc;
            } 
            else {
                rbuf[i] = result;
            }
        }
    }
    else {
        for (int i = 0; i < count; i++) {
            MPI_Send(sbuf + i, 1, type, root, i, MPI_COMM_WORLD);
        }
    }
    
    return 0;
}

int Scatter(void *sbuf_, int scount, MPI_Datatype stype, void *rbuf_, int rcount,
            MPI_Datatype rtype, int root, MPI_Comm comm, int rank, int n) {
    MPI_Status status;
    int *rbuf = rbuf_;
    int *sbuf = sbuf_;

    if (rank == root) {
        for (int i = 0; i < n; i++) {
            if (i != root) {
                MPI_Send(sbuf + i * scount, scount, stype, i, 0, MPI_COMM_WORLD);
            }
        }
        *rbuf = sbuf[root * scount];
    }
    else {
        MPI_Recv(rbuf, rcount, rtype, root, 0, MPI_COMM_WORLD, &status);
    }

    return 0;
}

int Gather(void *sbuf_, int scount, MPI_Datatype stype, void *rbuf_, int rcount,
            MPI_Datatype rtype, int root, MPI_Comm comm, int rank, int n) {
    MPI_Status status;
    int *rbuf = rbuf_;
    int *sbuf = sbuf_;

    if (rank == root) {
        for (int i = 0; i < n; i++) {
            if (i != root) {
                MPI_Recv(rbuf + i * rcount, rcount, rtype, i, 0, MPI_COMM_WORLD, &status);
            }
        }

        for (int i = 0; i < scount; i++) {
            rbuf[i] = sbuf[root * scount + i];
        }
    }
    else {
        MPI_Send(sbuf, scount, stype, root, 0, MPI_COMM_WORLD);
    }

    return 0;

}