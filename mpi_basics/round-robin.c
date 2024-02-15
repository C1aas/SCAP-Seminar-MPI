#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void round_robin(int rank, int num_procs);

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    int num_procs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Print off a hello world message
    printf("%d: hello (p=%d)\n", rank, num_procs);
    round_robin(rank, num_procs);
    printf("%d: goodbye\n", rank);
    // Finalize the MPI environment.
    MPI_Finalize();
    return 0;
}


void round_robin(int rank, int num_procs) {
    long int rand_own, rand_prev;
    int rank_next = (rank + 1) % num_procs;
    int rank_prev = rank == 0 ? num_procs - 1 : rank -1;
    MPI_Status status;

    srandom(time(NULL)+rank);
    rand_own = random() / (RAND_MAX / 100);
    printf("%d: my random value is: %ld\n", rank, rand_own);

    if(rank % 2 == 0) {
        printf("%d: sending %ld to %d\n", rank, rand_own, rank_next);
        MPI_Send((void*)&rand_own, 1, MPI_LONG, rank_next, 1, MPI_COMM_WORLD);

        printf("%d: receiving from %d\n", rank, rank_prev);
        MPI_Recv((void*)&rand_prev, 1, MPI_LONG, rank_prev, 1, MPI_COMM_WORLD, &status);
    } else {
        printf("%d: receiving from %d\n", rank, rank_prev);
        MPI_Recv((void*)&rand_prev, 1, MPI_LONG, rank_prev, 1, MPI_COMM_WORLD, &status);

        printf("%d: sending %ld to %d\n", rank, rand_own, rank_next);
        MPI_Send((void*)&rand_own, 1, MPI_LONG, rank_next, 1, MPI_COMM_WORLD);
    }
    printf("%d: I had %ld and %d send me %ld\n", rank, rand_own, rank_prev, rand_prev);
}
