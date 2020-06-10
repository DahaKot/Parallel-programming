#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>

typedef enum {
	OK = 1,
	FAIL = 0,
	SERVICE = 999999
} status_t;

typedef struct {
    int start;
    int end;
} task_t;

const int UNIT_LEN = 9;
const int UNIT_MAX = 999999999;
const int BLOCK_SIZE = 2;

int *getNumberFromString(FILE *input, int len);
void addNumbers(int *first_number, int *second_number, task_t localTask, int len, int start);
int countAddUpdateX(int *x_ptr);

int main(int argc, char* argv[]) {
	status_t argStatus = FAIL;
	MPI_Status status = {};
	int tasksNumber = 0;
	int blockSize = 0;
	task_t localTask = {};
	int rank = 0;
	int size = 0;
	double startTime = 0;
	double endTime = 0;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) {
		// Root branch
		//
		// Check arguments
		if (argc != 3) {
			printf("Need more arguments or / and more processes.\n");
			// Exit with fail status
			argStatus = FAIL;
			MPI_Bcast(&argStatus, 1, MPI_INT, 0, MPI_COMM_WORLD);
			MPI_Finalize();
			return 0;
		}

		// Read input data
        FILE *input = fopen(argv[1], "r");

        int len = -1;
        fscanf(input, "%d", &len);
        len = len / UNIT_LEN; // number of units with UNIT_LEN

        int *first_number = getNumberFromString(input, len);
        int *second_number = getNumberFromString(input, len);

        fclose(input);

		// Send status of arguments
		argStatus = OK;
		MPI_Bcast(&argStatus, 1, MPI_INT, 0, MPI_COMM_WORLD);

		// Send data
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(first_number, len, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(second_number, len, MPI_INT, 0, MPI_COMM_WORLD);

		// Prepare slave's tasks
		// block = UNIT_LEN * unit
		int blockSize = 2;
		int blocksNumber = len / blockSize; // should be at least 2
		int rest = len % blockSize;
		printf("Shedule : %d blocks(%d)", blocksNumber, blockSize);
		if (rest != 0) {
			blocksNumber++;
			printf(", 1 block(%d)", rest);
		} 
		else {
			// size of first block
			rest = blockSize;
		}
		printf("\n");

		task_t* taskShedule = malloc(blocksNumber * sizeof(task_t));
		
		// Add first block
		taskShedule[0].start = 0;
		taskShedule[0].end = rest;
		
		// Add other blocks
		int taskPtr = rest;
		for (int i = 1; i < blocksNumber; i++) {
			taskShedule[i].start = taskPtr;
			taskPtr += blockSize;
			taskShedule[i].end = taskPtr;
		}

		// Send blocks for work begin
		int blockPtr = 0;
		int buf = 0;
		int slave = 0;
		MPI_Barrier(MPI_COMM_WORLD);
		startTime = MPI_Wtime();

		for (blockPtr = blocksNumber - 1; blockPtr >= 0; blockPtr--) {
			// Find slave
			MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, SERVICE, MPI_COMM_WORLD, &status);
			slave = status.MPI_SOURCE;
			// Send block
			MPI_Send(&taskShedule[blockPtr], 2, MPI_INT, slave, OK, MPI_COMM_WORLD);
		}

		// End calc
		for (int i = 1; i < size; i++) {
			MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, SERVICE, MPI_COMM_WORLD, &status);
			slave = status.MPI_SOURCE;
			MPI_Send(taskShedule, 2, MPI_INT, slave, FAIL, MPI_COMM_WORLD);
		}

		int *cur_res = malloc((len + 2) * sizeof(int));
		int cur_add = 0;
		// Combine all the data together
		for (blockPtr = blocksNumber - 1; blockPtr >= 0; blockPtr--) {
			int start = taskShedule[blockPtr].start;
			int end = taskShedule[blockPtr].end;

			MPI_Recv(cur_res, len + 2, MPI_INT, MPI_ANY_SOURCE, start, MPI_COMM_WORLD, &status);

			// check if another result from slave needed
			int j = start;
			if (cur_add == 1 && cur_res[len + 1] > 0) {
				cur_add = cur_res[len + 1] - 1;
				cur_res[len] = cur_add;

				j = (end == len) ? rest : end;
				end = (end == len) ? blockSize : end + blockSize;
			}
			else if (cur_add == 1) {
				cur_res[end - 1]++;
			}

			for (int i = start; j < end; i++, j++) {
				second_number[i] = cur_res[j];
			}

			cur_add = cur_res[len];
		}

		FILE *output = fopen(argv[2], "w");
        if (cur_add) {
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
		MPI_Bcast(&argStatus, 1, MPI_INT, 0, MPI_COMM_WORLD);
		if (argStatus == FAIL) {
			MPI_Finalize();
			return 0;
		}
		
		// Get data
        int len = -1;
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        int start = len % BLOCK_SIZE;

        int *first_number = calloc(len, sizeof(int));
        int *second_number = calloc(len + 2, sizeof(int));

        MPI_Bcast(first_number, len, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(second_number, len, MPI_INT, 0, MPI_COMM_WORLD);

		// Calc loop
		MPI_Barrier(MPI_COMM_WORLD);
		startTime = MPI_Wtime();
		
		while (1) {
			// Ready for tasks
			MPI_Send(&rank, 1, MPI_INT, 0, SERVICE, MPI_COMM_WORLD);
			// Get block
			MPI_Recv(&localTask, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if (status.MPI_TAG != FAIL) {
				printf("[%d] Get %d..%d\n", rank, localTask.start, localTask.end - 1);
				// Calc task
		        addNumbers(first_number, second_number, localTask, len, start);

		        MPI_Send(second_number, len + 2, MPI_INT, 0, localTask.start, MPI_COMM_WORLD);
			} 
			else {
				break;
			}
		}
		endTime = MPI_Wtime();
		printf("[TIME %d] %lf\n", rank, endTime - startTime);
	}

	// Finish
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0) {
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

void addNumbers(int *first_number, int *second_number, task_t localTask, int len, int start) {
	int add = 0;
    int second_add = 0;
    int *additional_result = NULL;

    // second_number: ..............________........ (____ - is place to sum)
    // after function:
    // second_number: ..............________********^^ (***** - additional result and ^^ - two adds)
    // second_add = 0 == no additional_result
    // second_add = 1 == additional result is and add from it = 0
    // second_add = 2 == additional result is and add from it = 1

    for (int i = localTask.end - 1, k = i - localTask.start; i >= localTask.start; i--) {
        if (additional_result) {
            additional_result[k] = first_number[i] + second_number[i] + second_add - 1;

            second_add = countAddUpdateX(additional_result + i) + 1;
            k--;
        }

        second_number[i] = first_number[i] + second_number[i] + add;

        if (second_number[i] == UNIT_MAX && i == localTask.end - 1) {
        	additional_result = second_number + start;
        	if (localTask.end != len + start) {
		    	additional_result += localTask.end;
        	}

            additional_result[k] = 0;
            second_add = 2;
            k--;
        }

        add = countAddUpdateX(second_number + i);
    }

    second_number[len] = add;
    second_number[len + 1] = second_add;
}

int countAddUpdateX(int *x_ptr) {
    if (*x_ptr > UNIT_MAX) {
        *x_ptr = *x_ptr % (UNIT_MAX + 1);
        return 1;
    }
    return 0;
}