#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#include "game_of_life.h"
#include "arg_parser.h"
#include "image_creation.h"

#include <assert.h>

#include "utils.h"

#include <scorep/SCOREP_User.h>

/*
Optimizations
- Use a 2 bit to store the next state of the cell
- what to use this for?
    - execute 100000 random patterns and see which one lives the longest
- track how many cells live and then skip lines that are all dead and their neighbors too
- reduce Message size by factor 8 and use bitpacking in a char 

*/
/*
int main(int argc, char** argv) {
    GameConfig cfg = parseArguments(argc, argv);
    run(cfg);
    return 0;
}
*/

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}



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
MPI_Datatype workerConfigType;

void createWorkerConfigMPIType(MPI_Datatype *newtype) {
    int blocklengths[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    MPI_Datatype types[8] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint displacements[8];
    
    WorkerConfig temp;
    MPI_Aint base_address;
    MPI_Get_address(&temp, &base_address);
    MPI_Get_address(&temp.world_size, &displacements[0]);
    MPI_Get_address(&temp.world_rank, &displacements[1]);
    MPI_Get_address(&temp.row_index_main_grid, &displacements[2]);
    // Ignore local_grid
    MPI_Get_address(&temp.num_rows, &displacements[3]);
    MPI_Get_address(&temp.grid_width, &displacements[4]);
    MPI_Get_address(&temp.update_start_row, &displacements[5]);
    MPI_Get_address(&temp.update_end_row, &displacements[6]);
    MPI_Get_address(&temp.update_row_count, &displacements[7]);

    // Korrektur der Displacements
    for (int i = 0; i < 8; i++) {
        displacements[i] -= base_address;
    }

    MPI_Type_create_struct(8, blocklengths, displacements, types, newtype);
    MPI_Type_commit(newtype);
}

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
WorkerConfig initWorkerConfig(int world_size, int world_rank, int row_index_main_grid, int num_rows, int grid_width, int start_row) {
    assert(grid_width > 0);
    assert(num_rows > 0);
    WorkerConfig cfg = {
        .world_size = world_size,
        .world_rank = world_rank,
        .row_index_main_grid = row_index_main_grid,
        .num_rows = num_rows,
        .grid_width = grid_width,
        .update_start_row = -1,
        .update_end_row = -1,
        .update_row_count = -1,
        .start_row = start_row
    };
    cfg.update_start_row = world_rank == 1 ? 0 : 1; // worldrank 1 means first worker

    // minus 1 because its 0 indexed and minus 1 because the last row is not updated if its not the last rank
    cfg.update_end_row = world_rank == world_size - 1 ?  num_rows - 1 : num_rows - 1 - 1; 
    cfg.update_row_count = cfg.update_end_row - cfg.update_start_row + 1;
    return cfg;
}

WorkerConfig receiveWorkerConfig() {
    WorkerConfig cfg;
    MPI_Recv(&cfg, 1, workerConfigType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //print the config for debugging
    //printf("Rank %d: Received config: world_size: %d, world_rank: %d, row_index_main_grid: %d, num_rows: %d, grid_width: %d, update_start_row: %d, update_end_row: %d, update_row_count: %d\n", cfg.world_rank, cfg.world_size, cfg.world_rank, cfg.row_index_main_grid, cfg.num_rows, cfg.grid_width, cfg.update_start_row, cfg.update_end_row, cfg.update_row_count);
    return cfg;
}


/**
 * Updates the grid with the Game of Life rules, but only for the specified rows
 * it doesnt update the border of the grid
 * 
 * @param grid The grid to update
 * @param height The height of the grid
 * @param width The width of the grid
 * @param update_min The first row to update
 * @param update_max The last row to update
 */
void updateGridWithLimit(WorkerConfig cfg) {
    //unsigned char** grid, int height, int width, int update_min, int update_max
    assert(cfg.update_start_row >= 0);
    assert(cfg.update_end_row > 0);

    // First pass: Determine the next state for each cell
    // end row is inclusive
    for (int i = cfg.update_start_row; i <= cfg.update_end_row; i++) {
        for (int j = 0; j < cfg.grid_width; j++) {
            
            
            int liveNeighbors = 0;
            // Check all eight neighbors
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    if (y == 0 && x == 0) continue; // Skip the cell itself
                    int ni = i + y;
                    int nj = j + x;
                    if (ni >= 0 && ni < cfg.num_rows && nj >= 0 && nj < cfg.grid_width) {
                        // cells are either 
                        // 0b0 or 0b1 with bitshift we extract the one and add it to live Neighbors
                        liveNeighbors += cfg.local_grid[ni][nj] & 1;
                    }
                }
            }

            // Apply the Game of Life rules using the additional bits for the next state
            if (cfg.local_grid[i][j] & 1) { // Currently alive, correctly check using & 1
                if (liveNeighbors == 2 || liveNeighbors == 3) { // otherwise over or under population death
                    // a cell is alive 'x1' and next round it needs to live set 2nd bit to 1 '11'
                    cfg.local_grid[i][j] |= 2; // Set the second bit to stay alive in the next state
                }
            } else { // Currently dead
                if (liveNeighbors == 3) {
                    // a cell is dead 'x0' and next round it needs to be born set 2nd bit to 1 -> '10'
                    cfg.local_grid[i][j] |= 2; // Dead cell becoming alive, set the second bit
                }
            }
        }
    }
    
    // Second pass: Update the grid to the next state
    for (int i = cfg.update_start_row; i <= cfg.update_end_row; i++) {
        for (int j = 0; j < cfg.grid_width; j++) {
            cfg.local_grid[i][j] >>= 1; // Shift to the right to get the next state
        }
    }
}


/**
 * Receives the initial grid from the main process
 * 
 * @param cfg The worker process Config
 */
void receiveInitialGrid(WorkerConfig* cfg) {
    // printf("Rank %d: receiving %d rows\n", cfg->world_rank, cfg->num_rows);
    cfg->local_grid = createGrid(cfg->num_rows, cfg->grid_width);
    for (int i = 0; i < cfg->num_rows; i++) {
        //printf("Rank %d: Receiving row %d\n", world_rank, i);
        MPI_Recv(cfg->local_grid[i], cfg->grid_width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

}



/**
 * Sends the updated grid to the neighbor processes and receives the updated grid from the neighbor processes
 * 
 * @param cfg The worker process Config
 */
void sendandReceiveUpdatedGridRows(WorkerConfig cfg) {
    // Receive the updated grid
    
    // Send the updated grid to the neighbor processes
    MPI_Request requests[4]; // For non-blocking requests
    int request_count = 0;

    // lower upper in terms of index 0 is the lowest index and upper is a higher index

    int worker_idx = cfg.world_rank - 1;
    int worker_count = cfg.world_size - 1;
    if(worker_idx > 0) {
        //send border to the lower neighbor
        //printf("Rank %d: trying to send lower border to %d\n", cfg.world_rank, cfg.world_rank - 1);
        //printRow(cfg.world_rank, cfg.local_grid[cfg.update_start_row], cfg.grid_width);

        MPI_Isend(cfg.local_grid[cfg.update_start_row], cfg.grid_width, MPI_CHAR, cfg.world_rank - 1, 0, MPI_COMM_WORLD, &requests[request_count++]);
        //printf("Rank %d: Sent lower border to %d\n", cfg.world_rank, cfg.world_rank - 1);
    }
    //printf("Rank %d: worker_idx %d, worldsize: %d\n", cfg.world_rank, worker_idx, cfg.world_size);
    if(worker_idx < worker_count - 1) {
        //send border to the upper neighbor
        //printf("Rank %d: trying to send upper border to %d\n", cfg.world_rank, cfg.world_rank + 1);
        //printRow(cfg.world_rank, cfg.local_grid[cfg.update_end_row], cfg.grid_width);

        MPI_Isend(cfg.local_grid[cfg.update_end_row], cfg.grid_width, MPI_CHAR, cfg.world_rank + 1, 0, MPI_COMM_WORLD, &requests[request_count++]);
        //printf("Rank %d: Sent upper border to %d\n", cfg.world_rank, cfg.world_rank + 1);   
    }

    //printf("Rank %d: receiving\n", cfg.world_rank);
    if(worker_idx > 0) {
        //receive borders from the lower neighbor
        //printf("Rank %d: trying to receive lower border from %d\n", cfg.world_rank, cfg.world_rank - 1);
        MPI_Irecv(cfg.local_grid[0], cfg.grid_width, MPI_CHAR, cfg.world_rank - 1, 0, MPI_COMM_WORLD, &requests[request_count++]);
    }

    
    if(worker_idx < worker_count - 1) {
        //receive borders from the upper neighbor
        //printf("Rank %d: trying to receive upper border from %d\n", cfg.world_rank, cfg.world_rank + 1);
        MPI_Irecv(cfg.local_grid[cfg.num_rows - 1], cfg.grid_width, MPI_CHAR, cfg.world_rank + 1, 0, MPI_COMM_WORLD, &requests[request_count++]);
    }

    //printf("Rank %d waiting for %d non-blocking sends to complete\n", cfg.world_rank, request_count);
    // Wait for non-blocking sends to complete
    if (request_count > 0) {
        MPI_Waitall(request_count, requests, MPI_STATUSES_IGNORE);
    }


}

void sendGridToMain(WorkerConfig cfg) {
    
    // Send the updated grid to the main process;
    //printf("Rank %d: Sending total of %d rows to main\n", cfg.world_rank, cfg.update_row_count);

    // the update_end_row is inclusive
    for (int i = cfg.update_start_row; i <= cfg.update_end_row; i++) {
        MPI_Send(cfg.local_grid[i], cfg.grid_width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
}

/**
 * This is called by the main process 0. It receives the grid parts from all other worker processes
 * uses non blocking receive to receive the grid parts
 *  
 * @param grid The grid to receive the parts into
 * @param height The height of the grid
 * @param width The width of the grid
 * @param world_size The total number of processes
 */
void receiveGridParts(WorkerConfig* workerConfigs, unsigned char** grid, int height, int width, int world_size) {
    //printf("Main: Receiving total grid from workers\n");
    MPI_Request requests[height];

    //printf("Main: waiting to receive a total of %d rows\n", height);
    
    for (int worker_idx = 1; worker_idx < world_size; worker_idx++) {
        
        WorkerConfig* cfg = &workerConfigs[worker_idx-1];
        //printf("Main: waiting to receive %d rows from %d\n", cfg->update_row_count, worker_idx);
        for(int i = 0; i < cfg->update_row_count; i++) {
            //MPI_Recv(grid[cfg->row_index_main_grid + i], width, MPI_CHAR, worker_idx, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //printf("Main: Received row %d from %d\n", cfg->row_index_main_grid + i, worker_idx);
            MPI_Irecv(grid[cfg->row_index_main_grid + i], width, MPI_CHAR, worker_idx, 0, MPI_COMM_WORLD, &requests[worker_idx-1]);
        }
    }

    MPI_Waitall(world_size - 1, requests, MPI_STATUSES_IGNORE);
    
    //printf("Main: Received total grid\n");
}

void sendGridPartsToWorkers(unsigned char** grid, WorkerConfig* workerConfigs, int worker_count) {
    //printf("Main: Sending total grid to workers\n");

    for (int worker_idx = 0; worker_idx < worker_count; worker_idx++) {
        WorkerConfig* cfg = &workerConfigs[worker_idx];
        //printf("Main: Sending %d rows to %d\n", cfg->num_rows, cfg->world_rank);
        for(int i = 0; i < cfg->num_rows; i++) {
            //printf("Main: Sending row %d to %d\n", cfg->row_index_main_grid + i, cfg->world_rank);

            MPI_Send(grid[cfg->start_row + i], cfg->grid_width, MPI_CHAR, cfg->world_rank, 0, MPI_COMM_WORLD);
        }
    }
    //printf("Main: Sent total grid\n");

}

/**
 * Distributes the grid to all worker processes, and sends them their worker process Config
 * 
 * @param world_size The total number of processes
 * @param grid The grid to distribute
 * @param height The height of the grid
 * @param width The width of the grid
 * @param workerConfigs The worker process Configs
 */
void distributeAndSendConfig(int world_size, unsigned char** grid, int height, int width, WorkerConfig* workerConfigs){
    /*
    20 / 4 = 5
    start = rank * 5

    first = 0 - 4    | 0 * 5 - 0 + 5 -1
    second = 5 - 9   | 1 * 5 - 5 + 5 -1
    third = 10 - 14  | 2 * 5 - 10 + 5 -1
    fourth = 15 - 19
    */

    int worker_amount = world_size - 1;
    int rowsPerProcess = height / worker_amount;
    int remainder = height % worker_amount;
    // Distribute the grid to all processes, including itself
    for (int rank = 1; rank < world_size; rank++) { // iterate over all other processes
        
        int idx = rank - 1;
        int start_row = idx * rowsPerProcess + (idx < remainder ? idx : remainder);
        int end_row = start_row + rowsPerProcess - 1 + (idx < remainder ? 1 : 0);
        //printf("rank %d: start_row %d, end_row %d\n", rank, start_row, end_row);

        int row_index_main_grid = start_row;

        //send one line more to each process, therefor they are able to update the border
        start_row = max(start_row - 1, 0);
        end_row = min(end_row + 1, height - 1);
        
        int num_rows = end_row - start_row + 1;
        WorkerConfig cfg = initWorkerConfig(world_size, rank, row_index_main_grid, num_rows, width, start_row);
        workerConfigs[idx] = cfg;
        // send the config to the worker
        //printf("main -> %d: Sending config\n", rank);
        MPI_Send(&cfg, 1, workerConfigType, cfg.world_rank, 0, MPI_COMM_WORLD);
        printf("main -> %d: Sent config\n", cfg.world_rank);

        /*
        //printf("main -> %d: Sending %d rows\n", cfg.world_rank, cfg.num_rows);
        // Send the initial grid in chunks
        for(int i = 0; i < cfg.num_rows; i++) {
            // Send the initial grid in chunks
            //printf("Rank %d -> %d: Sending row %d\n", 0, rank, start_row + i);
            MPI_Send(grid[start_row + i], width, MPI_CHAR, cfg.world_rank, 0, MPI_COMM_WORLD);
        }
        */
    }

    //printf("main: Sent all Grids\n");
}


bool gridsAreEqual(unsigned char** grid_before, unsigned char** grid_after, int height, int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid_before[i][j] != grid_after[i][j]) {
                return false;
            }
        }
    }
    return true;
}

void copyGrid(unsigned char** src_grid, unsigned char** target_grid, int height, int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            target_grid[i][j] = src_grid[i][j];
        }
    }
}

void save_grid_as_image(char* name, unsigned char** grid, int height, int width, int iterations) {
    char file_name_buffer[80];
    sprintf(file_name_buffer, "grid_lifetime/%s-grid-%d-iter.jpg", name, iterations);
    write_jpeg_file(file_name_buffer, grid, width, height);
}

/**
 * Checks if the simulation has stopped
 * 
 * @param grid1 The first grid
 * @param grid2 The second grid
 * @param grid3 The third grid
 * @param grid4 The fourth grid
 * @param height The height of the grid
 * @param width The width of the grid
 * @return True if the simulation has stopped, false otherwise
*/
bool simulationStopped(unsigned char** grid1, unsigned char** grid2, unsigned char** grid3, unsigned char** grid4, int height, int width) {
    bool gridone = gridsAreEqual(grid1, grid2, height, width) || gridsAreEqual(grid1, grid3, height, width) || gridsAreEqual(grid1, grid4, height, width);
    bool gridtwo = gridsAreEqual(grid2, grid3, height, width) || gridsAreEqual(grid2, grid4, height, width);
    bool gridthree = gridsAreEqual(grid3, grid4, height, width);
    return gridone || gridtwo || gridthree;
}

int mainTryFindBigPattern(int argc, char** argv){
    MPI_Init(&argc, &argv);

    int height = 1000; // Example grid height
    int width = 1000; // Example grid width
    int iterations = 5000; // Example number of iterations

    int world_rank, world_size;  

    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    //register custom MPI type
    createWorkerConfigMPIType(&workerConfigType);

    // create a new communicator for the workers
    MPI_Comm worker_comm;
    int color = (world_rank == 0) ? MPI_UNDEFINED : 1; // Same color for workers, MPI_UNDEFINED for the main process
    MPI_Comm_split(MPI_COMM_WORLD, color, world_rank, &worker_comm); // All processes call MPI_Comm_split, but only worker processes get a valid new communicator

    if (world_rank == 0) {
        printf("main: Starting...\n");
        long long start = current_time_millis();

        unsigned char** grid = createGrid(height, width);
        initializeGridRandom(grid, height, width, 0.3);

        printf("main: Grid generated\n");

        unsigned char** initial_grid = createGrid(height, width);
        copyGrid(grid, initial_grid, height, width);
        
        //initializeGridModulo(grid, height, width, 3);
        //initializeGridZero(grid, height, width);
        

        WorkerConfig workerConfigs[world_size - 1];
        distributeAndSendConfig(world_size, grid, height, width, workerConfigs);
        sendGridPartsToWorkers(grid, workerConfigs, world_size-1);

        //printf("main: Writing initial image to 'mpi_initial_grid.jpg'\n");
        //write_jpeg_file("mpi_initial_grid.jpg", initial_grid, width, height);

        unsigned char** grid_temp1 = createGrid(height, width);
        unsigned char** grid_temp2 = createGrid(height, width);
        unsigned char** grid_temp3 = createGrid(height, width);
        unsigned char** grid_temp4 = createGrid(height, width);
 
        for(int i = 0; i < iterations; i++) {
             if (i % 10 == 0) {
                receiveGridParts(workerConfigs, grid_temp1, height, width, world_size);
            }
            if (i % 10 == 1) {
                receiveGridParts(workerConfigs, grid_temp2, height, width, world_size);
            }
            if (i % 10 == 2) {
                receiveGridParts(workerConfigs, grid_temp3, height, width, world_size);
            }
            if (i % 10 == 3) {
                receiveGridParts(workerConfigs, grid_temp4, height, width, world_size);
                if(simulationStopped(grid_temp1, grid_temp2, grid_temp3, grid_temp4, height, width)) {
                    printf("Grid is the same as before after %d iterations\n", i );
                    if (i > 2000) {
                        printf("*** FOUND A GOOD PATTERN ***\n");
                        //printf("Found a pattern that repeats after %d iterations\n", i);
                        writeGridToFile("initial_grid", initial_grid, height, width, i);
                        save_grid_as_image("initial_grid", initial_grid, height, width, i);
                        save_grid_as_image("final_grid", grid_temp3, height, width, i);
                    }

                    //write_jpeg_file("mpi_result_grid.jpg", grid_temp3, width, height);
                    MPI_Abort(MPI_COMM_WORLD, 300);
                    break;
                    
                }
            }
        }
        
        freeGrid(grid, height);
        freeGrid(initial_grid, height);
        freeGrid(grid_temp1, height);
        freeGrid(grid_temp2, height);
        freeGrid(grid_temp3, height);
        
    } else {
        //printf("Rank %d: Receiving config\n", world_rank);
        WorkerConfig cfg = receiveWorkerConfig();
        //printf("Rank %d: Received config\n", world_rank);
        receiveInitialGrid(&cfg);
        //printf("Rank %d: Received config and initial grid. Starting to calculate...\n", world_rank);

        MPI_Barrier(worker_comm); // Barrier operation for workers only, to make them start at the same time

        for(int i = 0; i < iterations; i++) {
            updateGridWithLimit(cfg);
            
            //printf("Rank %d: Updated grid\n", world_rank);
            //printGrid(cfg.local_grid, cfg.num_rows, cfg.grid_width, 0);
            sendandReceiveUpdatedGridRows(cfg);
            //printf("Rank %d: Sent and received updated grid\n", world_rank);
            //printGrid(cfg.local_grid, cfg.num_rows, cfg.grid_width, 0);
            if (i % 10 == 0) {
                sendGridToMain(cfg);
            }
            if (i % 10 == 1) {
                sendGridToMain(cfg);
            }
            if (i % 10 == 2) {
                sendGridToMain(cfg);
            }
            if (i % 10 == 3) {
                sendGridToMain(cfg);
            }
            // if(i % 100 == 0) {
            //     printf("Rank %d: Iteration %d\n", world_rank, i);
            // }
        }
        freeGrid(cfg.local_grid, cfg.num_rows);

        //printf("Rank %d: calculated %d iterations\n", world_rank, iterations);
        //sendGridToMain(cfg);
        //printf("Rank %d: done\n", world_rank);

        MPI_Comm_free(&worker_comm); // Free the communicator for worker processes
    }

    // free custom MPI type
    MPI_Type_free(&workerConfigType);

    MPI_Finalize();

    

    
    return 0;
}

int main(int argc, char** argv) {
    SCOREP_RECORDING_OFF();
    MPI_Init(&argc, &argv);

    int height = 10000; // Example grid height
    int width = 10000; // Example grid width
    int iterations = 100; // Example number of iterations

    int world_rank, world_size;  

    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    assert(height / world_size > 2);

    //register custom MPI type
    createWorkerConfigMPIType(&workerConfigType);

    SCOREP_RECORDING_ON();

    
    // create a new communicator for the workers
    MPI_Comm worker_comm;
    int color = (world_rank == 0) ? MPI_UNDEFINED : 1; // Same color for workers, MPI_UNDEFINED for the main process
    MPI_Comm_split(MPI_COMM_WORLD, color, world_rank, &worker_comm); // All processes call MPI_Comm_split, but only worker processes get a valid new communicator
    

    if (world_rank == 0) {
        printf("main: Starting...\n");
        long long start = current_time_millis();

        unsigned char** grid = createGrid(height, width);

        initializeGridRandom(grid, height, width, 0.3);
        //initializeGridModulo(grid, height, width, 3);
        //initializeGridZero(grid, height, width);

        printf("main: Grid generated\n");
        
        WorkerConfig workerConfigs[world_size - 1];
        distributeAndSendConfig(world_size, grid, height, width, workerConfigs);
        printf("main: Sent all Configs\n");
        sendGridPartsToWorkers(grid, workerConfigs, world_size-1);
        printf("main: Sent all Grids\n");

        printf("main: Writing initial image to 'mpi_initial_grid.jpg'\n");
        write_jpeg_file("mpi_initial_grid.jpg", grid, width, height);

        receiveGridParts(workerConfigs, grid, height, width, world_size);

        printf("main: Received total grid from workers\n");
        printf("main: Writing image to 'mpi_result_grid.jpg'\n");
        write_jpeg_file("mpi_result_grid.jpg", grid, width, height);
        
        printf("main: done\n");
        long long end = current_time_millis();
        long long total = end - start;

        printf("\n----------------------------------------\n");
        printf("Time taken: %lld milliseconds\n", total);
        printf("Time per iteration: %lld milliseconds\n", total / iterations);
        printf("----------------------------------------\n");
        
        freeGrid(grid, height);
        
    } else {
        //printf("Rank %d: Receiving config\n", world_rank);
        WorkerConfig cfg = receiveWorkerConfig();
        printf("Rank %d: Received config\n", world_rank);
        receiveInitialGrid(&cfg);
        printf("Rank %d: Received initial grid. Starting to calculate...\n", world_rank);

        MPI_Barrier(worker_comm); // Barrier operation for workers only, to make them start at the same time

        for(int i = 0; i < iterations; i++) {
            updateGridWithLimit(cfg);
            
            //printf("Rank %d: Updated grid\n", world_rank);
            //printGrid(cfg.local_grid, cfg.num_rows, cfg.grid_width, 0);
            sendandReceiveUpdatedGridRows(cfg);
            //printf("Rank %d: Sent and received updated grid\n", world_rank);

        }
        sendGridToMain(cfg);
        freeGrid(cfg.local_grid, cfg.num_rows);

        //printf("Rank %d: calculated %d iterations\n", world_rank, iterations);
        //sendGridToMain(cfg);
        printf("Rank %d: worker done\n", world_rank);

        MPI_Comm_free(&worker_comm); // Free the communicator for worker processes
    }

    SCOREP_RECORDING_OFF();
    // free custom MPI type
    MPI_Type_free(&workerConfigType);

    MPI_Finalize();

    return 0;
}

