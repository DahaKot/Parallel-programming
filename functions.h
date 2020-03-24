#include <mpi.h>
#include <stdio.h>
#include <math.h>

void Print_Buf(int *buf, int len, int rank);
double Count_std(double mean_time, double *times, int n_loops);

int Bcast(void *buf, int count, MPI_Datatype type, int root, 
            MPI_Comm comm, int rank, int n);
int Apply(MPI_Op op, void *buf, int count, int *loc);
int Reduce(void *sbuf, void *rbuf, int count, MPI_Datatype type, MPI_Op op, 
            int root, MPI_Comm comm, int rank, int n);
int Scatter(void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount,
            MPI_Datatype rtype, int root, MPI_Comm comm, int rank, int n);
int Gather(void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount,
            MPI_Datatype rtype, int root, MPI_Comm comm, int rank, int n);