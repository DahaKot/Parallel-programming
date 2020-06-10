#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h> 

typedef enum {
    OK = 0,
    FAIL = 1
} status_t;

typedef struct {
    int *result;
    int add;
} add_result;

typedef struct {
    int start;
    int end;
} task_t;

const int UNIT_LEN = 9;
const int UNIT_MAX = 999999999;

int *getNumberFromString(FILE *input, int len);
add_result addNumbers(int *first_number, int *second_number, task_t localTask, int len, int *add);
int countAddUpdateX(int *x_ptr);

int main(int argc, char* argv[]) {
    task_t localTask = {};
    
    double startTime = 0;
    double endTime = 0;

    MPI_Status recv_status = {};
    status_t status = FAIL;

    MPI_Init(&argc, &argv);

    int size = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        // Root branch
        //
        // Check arguments
        if (argc != 3 && size > 1) {
            printf("Need more arguments or / and more processes.\n");
            // Exit with fail status
            status = FAIL;
            MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Finalize();
            return 0;
        }

        startTime = MPI_Wtime();
        // Read input data
        FILE *input = fopen(argv[1], "r");

        int len = -1;
        fscanf(input, "%d", &len);
        len = len / UNIT_LEN; // number of units with UNIT_LEN

        int *first_number = getNumberFromString(input, len);
        int *second_number = getNumberFromString(input, len);

        fclose(input);

        // Send status of arguments
        status = OK;
        MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Send data
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(first_number, len, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(second_number, len, MPI_INT, 0, MPI_COMM_WORLD);

        // Prepare slave's tasks
        int localLen = len / (size - 1);
        int rest = len % (size - 1);
        task_t *taskShedule = malloc(size * sizeof(task_t));
        int *sizes = malloc(size * sizeof(int));
        int *places = malloc(size * sizeof(int));

        int taskPointer = 0;
        printf("Shedule : ");
        for (int i = 1; i < size; i++) {
            taskShedule[i].start = taskPointer;
            taskPointer += localLen;
            if (i < (rest + 1)) {
                taskPointer++;
            }
            taskShedule[i].end = taskPointer;

            printf("[%d] %d(%d..%d) ", i, taskShedule[i].end - taskShedule[i].start\
                                    , taskShedule[i].start, taskShedule[i].end - 1);
        }

        // Send tasks
        MPI_Scatter(taskShedule, 2, MPI_INT, &localTask, 2, MPI_INT, 0, MPI_COMM_WORLD);

        // Send a start to collect add-s
        int add_needed = 0;
        MPI_Send(&add_needed, 1, MPI_INT, size - 1, OK, MPI_COMM_WORLD);

        // Combine results of every process
        sizes[0] = 0;
        places[0] = 0;
        for (int i = 1; i < size; i++) {
            sizes[i] = taskShedule[i].end - taskShedule[i].start;
            places[i] = places[i - 1] + sizes[i - 1];
        }

        MPI_Gatherv(NULL, 0, MPI_INT, second_number, sizes, places, MPI_INT, 0, MPI_COMM_WORLD);

        MPI_Recv(&add_needed, 1, MPI_INT, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_status);

        FILE *output = fopen(argv[2], "w");
        if (add_needed) {
            fprintf(output, "%d", 1);
        }

        // print the end result
        for (int i = 0; i < len; i++) {
            fprintf(output, "%09d", second_number[i]);
        }

        fclose(output);
    }
    else {
        // Slave branch
        //
        // Check status
        MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (status == FAIL) {
            MPI_Finalize();
            return 0;
        }

        // Get data
        int len = -1;
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);

        int *first_number = calloc(len, sizeof(int));
        int *second_number = calloc(len, sizeof(int));

        MPI_Bcast(first_number, len, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(second_number, len, MPI_INT, 0, MPI_COMM_WORLD);

        // Get tasks
        MPI_Scatter(NULL, 0, 0, &localTask, 2, MPI_INT, 0, MPI_COMM_WORLD);
        // printf("[%d] Get %d..%d\n", rank, localTask.start, localTask.end - 1);

        // Calc task
        int add = 0;
        startTime = MPI_Wtime();

        add_result additional_result = addNumbers(first_number, second_number, localTask, len, &add);

        endTime = MPI_Wtime();

        // decide what result to send beased on previous processes result
        int add_needed = 0;
        int prev_rank = (rank == size - 1) ? 0 : rank + 1;

        MPI_Recv(&add_needed, 1, MPI_INT, prev_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_status);
        int *send_buf = second_number + localTask.start;
        
        if (add_needed && additional_result.result) {
            send_buf = additional_result.result;
            add = additional_result.add;
        }
        else if (add_needed) {
            second_number[localTask.end - 1]++;
        }
        MPI_Send(&add, 1, MPI_INT, rank - 1, OK, MPI_COMM_WORLD);

        // send the result to the root
        MPI_Gatherv(send_buf, localTask.end - localTask.start, MPI_INT,\
                    NULL, NULL, NULL, MPI_INT, 0, MPI_COMM_WORLD);

        printf("[TIME %d] %lf\n", rank, endTime - startTime);
    }


    // Finish
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
    {
        endTime = MPI_Wtime();
        printf("[TIME RES] %lf\n", endTime - startTime);
    }

    MPI_Finalize();
    return 0;
}

int *getNumberFromString(FILE *input, int len) {
    int *number = malloc(len * sizeof(int));

    for (int i = 0; i < len; i++) {
        fscanf(input, "%9d", number + i);
    }

    return number;
}

add_result addNumbers(int *first_number, int *second_number, task_t localTask, int len, int *add) {
    int second_add = 0;
    int *additional_result = NULL;

    for (int i = localTask.end - 1, k = i - localTask.start; i >= localTask.start; i--) {
        if (additional_result) {
            additional_result[k] = first_number[i] + second_number[i] + second_add;

            second_add = countAddUpdateX(additional_result + i);
            k--;
        }

        second_number[i] = first_number[i] + second_number[i] + *add;

        if (second_number[i] == UNIT_MAX && i == localTask.end - 1) {
            additional_result = malloc((localTask.end - localTask.start) * sizeof(int));
            additional_result[k] = 0;
            second_add = 1;
            k--;
        }

        *add = countAddUpdateX(second_number + i);
    }

    add_result result = {additional_result, second_add};

    return result;
}

int countAddUpdateX(int *x_ptr) {
    if (*x_ptr > UNIT_MAX) {
        *x_ptr = *x_ptr % (UNIT_MAX + 1);
        return 1;
    }
    return 0;
}