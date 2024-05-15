#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mpi.h>

#include <scorep/SCOREP_User.h>


#include "game_of_life_mpi.h"
#include "arg_parser.h"
#include "image_creation.h"
#include "utils.h"
#include "utils_grid.h"


/*
Optimizations
- Use a 2 bit to store the next state of the cell
- what to use this for?
    - execute 100000 random patterns and see which one lives the longest
- track how many cells live and then skip lines that are all dead and their neighbors too
- reduce Message size by factor 8 and use bitpacking in a char 

*/


void masterProcess(int argc, char** argv, int world_size);
void workerProcess(MPI_Comm worker_comm, int world_rank);

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int world_rank, world_size;  
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    //assure world size is at least 2
    if (world_size < 2) {
        printf("World size must be at least 2 for %s to work\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    //register custom MPI type
    createWorkerConfigMPIType(&workerConfigType);
    
    // create a new communicator for the workers
    MPI_Comm worker_comm;
    int color = (world_rank == 0) ? MPI_UNDEFINED : 1; // Same color for workers, MPI_UNDEFINED for the main process
    
    // All processes call MPI_Comm_split, 
    // but only worker processes  get a valid new communicator
    MPI_Comm_split(MPI_COMM_WORLD, color, world_rank, &worker_comm); 


    if (world_rank == 0) {
        masterProcess(argc, argv, world_size);
    } else {
        workerProcess(worker_comm, world_rank);
    }

    MPI_Comm_free(&worker_comm); // Free the communicator for worker processes

    // free custom MPI type
    MPI_Type_free(&workerConfigType);

    MPI_Finalize();

    return 0;
    
}

void masterProcess(int argc, char** argv, int world_size) {
    debugPrint("Master process: Starting...\n");
    GameConfig cfg = parseArguments(argc, argv);
    double start_time, end_time;
    start_time = MPI_Wtime();

    unsigned char** grid = createGrid(cfg.height, cfg.width);
    char file_name_buffer[80];

    printf("Master process: initializing grid\n");
    initializeGridRandom(grid, cfg.height, cfg.width, 0.3);
    //initializeGridModulo(grid, height, width, 3);
    //initializeGridZero(grid, height, width);

    printf("Master process: Grid generated\n");
    
    WorkerConfig workerConfigs[world_size - 1];
    distributeAndSendConfig(world_size, cfg.height, cfg.width, cfg.total_iterations, workerConfigs);
    
    printf("Master process: Sent all Configs\n");
    sendGridPartsToWorkers(grid, workerConfigs, world_size-1);
    printf("Master process: Sent all Grids\n");

    debugPrint("Master process: Writing initial image to 'mpi_initial_grid.jpg'\n");

    
    
    snprintf(file_name_buffer, 80, "mpi_initial_grid-%d-%dx%d.jpg", cfg.total_iterations, cfg.width, cfg.height);
    write_jpeg_file(file_name_buffer, grid, cfg.width, cfg.height);

    receiveGridParts(workerConfigs, grid, cfg.height, cfg.width, world_size);
    debugPrint("Master process: Received total grid from workers\n");


    debugPrint("Master process: Writing image to 'mpi_result_grid.jpg'\n");
    snprintf(file_name_buffer, 80, "mpi_result_grid-%d-%dx%d.jpg", cfg.total_iterations, cfg.height, cfg.width);
    write_jpeg_file(file_name_buffer, grid, cfg.width, cfg.height);
    

    freeGrid(grid, cfg.height);

    end_time = MPI_Wtime();
    printf("Master process time: %f seconds\n", end_time - start_time);
}


void workerProcess(MPI_Comm worker_comm, int world_rank) {
    debugPrint("Worker process %d started\n", world_rank);
    double start_time, end_time;
    start_time = MPI_Wtime();
    
    WorkerConfig cfg = receiveWorkerConfig();
    debugPrint("Rank %d: Received config\n", world_rank);
    receiveInitialGrid(&cfg);
    debugPrint("Rank %d: Received initial grid. Starting to calculate...\n", world_rank);

    MPI_Barrier(worker_comm); // Barrier operation for workers only, to make them start at the same time

    for(int i = 0; i < cfg.total_iterations; i++) {
        updateGridWithLimit(cfg);
        sendandReceiveUpdatedGridRows(cfg);
    }

    sendGridToMain(cfg);
    freeGrid(cfg.local_grid, cfg.num_rows);

    end_time = MPI_Wtime();
    double timePerIteration = (end_time - start_time) / cfg.total_iterations;
    printf("Worker process %2d time: %f seconds timePerIteration: %f ms\n", world_rank, end_time - start_time, timePerIteration*1000);
}