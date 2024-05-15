#ifndef GAME_OF_LIFE_MPI_H
#define GAME_OF_LIFE_MPI_H

#include <mpi.h>

/**
 * Data of the worker process
 * 
 * @param world_rank The rank of the worker
 * @param world_size The total number of processes
 * @param local_grid The local grid of the worker
 * @param grid_height The height of the grid
 * @param grid_width The width of the grid
 * @param update_start_row The first row to update
 * @param update_end_row The last row to update
 */
struct WorkerConfig {
    int world_size;
    int world_rank;
    int total_iterations;

    int row_index_main_grid; // the row index in the main grid of the update_start_row
    unsigned char** local_grid;
    int num_rows; //dynamic height of the grid, more than the update update_row_count
    int grid_width; // total width of the grid, stays the same for all workers

    int update_start_row; // most of the time its 1 but if its the first rank it is 0
    int update_end_row; // most of the time its height - 1 but if its the last rank it is height
    int update_row_count;

    //only used for sending the intital grid to the workers
    int start_row;  // the start row of the local grid in the main grid. The local grid is one larger than the grid thats getting updated
};
typedef struct WorkerConfig WorkerConfig;

//create custom global MPI type for the worker config
extern MPI_Datatype workerConfigType;

void createWorkerConfigMPIType(MPI_Datatype *newtype);

/**
 * Initializes the worker process Config
 * sets the world_rank, world_size, grid_width and the update_start_row and update_end_row
 * 
 * @param world_size The total number of processes
 * @param world_rank The rank of the worker
 * @param row_index_main_grid The row index in the main grid that will be send back to the main process
 * @param num_rows The number of rows to update
 * @param width The width of the grid
 * @return The initialized worker process Config
 */
WorkerConfig initWorkerConfig(int world_size, int world_rank, int row_index_main_grid, int num_rows, int grid_width, int start_row, int total_iterations);


/**
 * Distributes the Config to all worker processes
 * 
 * @param world_size The total number of processes
 * @param grid The grid to distribute
 * @param height The height of the grid
 * @param width The width of the grid
 * @param workerConfigs The worker process Configs
 */
void distributeAndSendConfig(int world_size, int height, int width, int total_iterations, WorkerConfig* workerConfigs);


/**
 * Receives the Config from the main process
 * 
 * @return The worker process Config
*/
WorkerConfig receiveWorkerConfig();


/**
 * Receives the initial grid from the main process
 * 
 * @param cfg The worker process Config
 */
void receiveInitialGrid(WorkerConfig* cfg);


/**
 * Updates the grid with the Game of Life rules, but only for the specified rows
 * it doesnt update the border of the grid.
 * This is a variation of the single-core implementation 
 * 
 * @param grid The grid to update
 * @param height The height of the grid
 * @param width The width of the grid
 * @param update_min The first row to update
 * @param update_max The last row to update
 */
void updateGridWithLimit(WorkerConfig cfg);


/**
 * Sends the updated grid to the neighbor processes and receives the updated grid from the neighbor processes
 * 
 * @param cfg The worker process Config
 */
void sendandReceiveUpdatedGridRows(WorkerConfig cfg);


/**
 * Sends the grid to the main process from the worker process
 * 
 * @param cfg The worker process Config
 
*/
void sendGridToMain(WorkerConfig cfg);

/**
 * This is called by the main process 0. It receives the grid parts from all other worker processes
 * uses non blocking receive to receive the grid parts
 *  
 * @param grid The grid to receive the parts into
 * @param height The height of the grid
 * @param width The width of the grid
 * @param world_size The total number of processes
 */
void receiveGridParts(WorkerConfig* workerConfigs, unsigned char** grid, int height, int width, int world_size);

/**
 * This is called by the main process 0. It sends the grid parts to all worker processes
 * 
 * @param grid The grid to send the parts from
 * @param workerConfigs The worker process Configs
 * @param worker_count The total number of worker processes
 */
void sendGridPartsToWorkers(unsigned char** grid, WorkerConfig* workerConfigs, int worker_count);

#endif