#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "game_of_life.h"
#include "arg_parser.h"

#include <assert.h>

/*
Optimizations
- Use a 2 bit to store the next state of the cell

*/
/*
int main(int argc, char** argv) {
    GameConfig cfg = parseArguments(argc, argv);
    run(cfg);
    return 0;
}
*/


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
    int start_row; // start row in the global grid
    int end_row; // end row in the global grid
    unsigned char** local_grid;
    int grid_height; // total height of the grid
    int grid_width; // total width of the grid
    int update_start_row; // The first row to update
    int update_end_row; // The last row to update
};
typedef struct WorkerConfig WorkerConfig;



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
    for (int i = cfg.update_start_row; i < cfg.update_end_row; i++) {
        for (int j = 0; j < cfg.grid_width; j++) {
            cfg.local_grid[i][j] = cfg.world_rank;
            /*
            int liveNeighbors = 0;
            // Check all eight neighbors
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    if (y == 0 && x == 0) continue; // Skip the cell itself
                    int ni = i + y;
                    int nj = j + x;
                    if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                        // cells are either 
                        // 0b0 or 0b1 with bitshift we extract the one and add it to live Neighbors
                        liveNeighbors += grid[ni][nj] & 1;
                    }
                }
            }

            // Apply the Game of Life rules using the additional bits for the next state
            if (grid[i][j] & 1) { // Currently alive, correctly check using & 1
                if (liveNeighbors == 2 || liveNeighbors == 3) { // otherwise over or under population death
                    // a cell is alive 'x1' and next round it needs to live set 2nd bit to 1 '11'
                    grid[i][j] |= 2; // Set the second bit to stay alive in the next state
                }
            } else { // Currently dead
                if (liveNeighbors == 3) {
                    // a cell is dead 'x0' and next round it needs to be born set 2nd bit to 1 -> '10'
                    grid[i][j] |= 2; // Dead cell becoming alive, set the second bit
                }
            }*/
        }
    }

    
    /*
    // Second pass: Update the grid to the next state
    for (int i = cfg.update_start_row; i < cfg.update_end_row; i++) {
        for (int j = 0; j < width; j++) {
            grid[i][j] >>= 1; // Shift to the right to get the next state
        }
    }
    */
    

}

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}



/**
 * Initializes the worker process Config
 * sets the world_rank, world_size, grid_width and the update_start_row and update_end_row
 * 
 * @param world_rank The rank of the worker
 * @param world_size The total number of processes
 * @param grid_width The width of the grid
 * @return The configuration for the worker
 */
WorkerConfig initWorkerConfig(int world_rank, int world_size, int grid_width) {
    assert(grid_width > 0);

    WorkerConfig cfg = {
        .world_rank = world_rank,
        .world_size = world_size,
        .local_grid = NULL,
        .grid_height = -1,
        .grid_width = grid_width,
        .update_start_row = -1,
        .update_end_row = -1,
        .start_row = -1,
        .end_row = -1
    };

    // set the start and end row to update
    cfg.update_start_row = cfg.world_rank == 0 ? 0 : 1; // only if first rank, update first row, otherwise update from second row on

    //printf("Rank %d: initialized\n", cfg.world_rank);
    return cfg;
}

void receiveInitialGrid(WorkerConfig* cfg) {
    // Receive the grid height
    int numRows;
    MPI_Recv(&numRows, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    cfg->grid_height = numRows;

    cfg->update_end_row = cfg->world_rank == cfg->world_size - 1 ?  cfg->grid_height : cfg->grid_height - 1; // only if last rank, update last row, otherwise update until second last row

    assert(cfg->grid_height > 0);

    printf("Rank %d: receiving %d rows\n", cfg->world_rank, cfg->grid_height);
    cfg->local_grid = createGrid(cfg->grid_height, cfg->grid_width);
    for (int i = 0; i < numRows; i++) {
        //printf("Rank %d: Receiving row %d\n", world_rank, i);
        MPI_Recv(cfg->local_grid[i], cfg->grid_width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    printf("Rank %d: received grid\n", cfg->world_rank);
}

void sendandReceiveUpdatedGrid(WorkerConfig cfg) {
    // Receive the updated grid
    
    // Send the updated grid to the neighbor processes
    MPI_Request send_request[2]; // For non-blocking sends
    int send_count = 0;

    if(cfg.world_rank > 0) {
        //send borders to the lower neighbor
        MPI_Isend(cfg.local_grid[0], cfg.grid_width, MPI_CHAR, cfg.world_rank - 1, 0, MPI_COMM_WORLD, &send_request[send_count++]);
    }

    if(cfg.world_rank < cfg.world_size - 1) {
        //send borders to the upper neighbor
        MPI_Isend(cfg.local_grid[cfg.grid_height - 1], cfg.grid_width, MPI_CHAR, cfg.world_rank + 1, 0, MPI_COMM_WORLD, &send_request[send_count++]);
    }

    
    if(cfg.world_rank > 0) {
        //receive borders from the lower neighbor
        MPI_Recv(cfg.local_grid[0], cfg.grid_width, MPI_CHAR, cfg.world_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    
    if(cfg.world_rank < cfg.world_size - 1) {
        //receive borders from the upper neighbor
        MPI_Recv(cfg.local_grid[cfg.grid_height - 1], cfg.grid_width, MPI_CHAR, cfg.world_rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Wait for non-blocking sends to complete
    if (send_count > 0) {
        MPI_Waitall(send_count, send_request, MPI_STATUSES_IGNORE);
    }


}

void sendGridToMain(WorkerConfig cfg) {
    
    // Send the updated grid to the main process
    int totalRows = cfg.update_end_row - cfg.update_start_row + 1;
    printf("Rank %d: Sending total of %d rows to main\n", cfg.world_rank, totalRows);
    for (int i = 0; i < totalRows; i++) {
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
    printf("Main: Receiving total grid from workers\n");
    MPI_Request requests[world_size-1];

    //TODO not only receive one row but all rows from each worker
    for (int worker_idx = 1; worker_idx < world_size; worker_idx++) {
        
        WorkerConfig* cfg = &workerConfigs[worker_idx-1];
        int total_rows = cfg->end_row - cfg->start_row + 1;
        printf("Main: waiting to receiv %d rows from %d\n", total_rows, worker_idx,);
        for(int i = 0; i < total_rows; i++) {

            int target_row = cfg->start_row + i;
            assert(target_row <= cfg->end_row);

            MPI_Irecv(grid[target_row], width, MPI_CHAR, worker_idx, 0, MPI_COMM_WORLD, &requests[worker_idx-1]);
        }
    }

    MPI_Waitall(world_size - 1, requests, MPI_STATUSES_IGNORE);
    
    printf("Main: Received total grid\n");
    printGrid(grid, height, width, 0);
}
//TODO the end and start row dont match the actual rows that are sent
//fix the send and receive


void distributeAndSendGrid(int world_size, unsigned char** grid, int height, int width, WorkerConfig* workerConfigs){
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
        workerConfigs[rank - 1] = initWorkerConfig(rank, world_size, width);  // initialize the worker config to be able to receive the grid
        int idx = rank - 1;
        int startRow = idx * rowsPerProcess + (idx < remainder ? idx : remainder);
        int endRow = startRow + rowsPerProcess - 1 + (idx < remainder ? 1 : 0);
        printf("rank %d: startRow %d, endRow %d\n", rank, startRow, endRow);

        //send one line more to each process, therefor they are able to update the border
        startRow = max(startRow - 1, 0);
        endRow = min(endRow + 1, height - 1);
        
        int numRows = endRow - startRow + 1;

        // save it to be able to receive and stitch the grid later
        workerConfigs[rank - 1].start_row = startRow;
        workerConfigs[rank - 1].end_row = endRow;
        workerConfigs[rank - 1].grid_height = numRows;

        printf("main -> %d: Sending %d rows\n", rank, numRows);
        MPI_Send(&numRows, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
        for(int i = 0; i < numRows; i++) {
            // Send the initial grid in chunks
            //printf("Rank %d -> %d: Sending row %d\n", 0, rank, startRow + i);
            MPI_Send(grid[startRow + i], width, MPI_CHAR, rank, 0, MPI_COMM_WORLD);
        }
        
        //MPI_Send(grid + startRow * width, numRows * width, MPI_INT, rank, 0, MPI_COMM_WORLD);
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int height = 20; // Example grid height
    int width = 20; // Example grid width

    int world_rank, world_size;  

    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);


    if (world_rank == 0) {
        
        unsigned char** grid = createGrid(height, width);
    
        initializeGridRandom(grid, height, width, 0.3);
        printf("Total grid\n");
        printGrid(grid, height, width, 0);

        WorkerConfig workerConfigs[world_size - 1];
        distributeAndSendGrid(world_size, grid, height, width, workerConfigs);

        receiveGridParts(workerConfigs, grid, height, width, world_size);
        printf("Total grid\n");
        printGrid(grid, height, width, 0);

    } else {
        
        WorkerConfig cfg = initWorkerConfig(world_rank, world_size, width);
        receiveInitialGrid(&cfg);
        
        //printf("Rank %d: Received grid\n", world_rank);
        //printGrid(local_grid, numRows, width, 0);
        updateGridWithLimit(cfg);
        printf("Rank %d: Updated grid\n", world_rank);
        printGrid(cfg.local_grid, cfg.grid_height, cfg.grid_width, 0);
        
        //sendandReceiveUpdatedGrid(cfg);

        sendGridToMain(cfg);

    }

    MPI_Finalize();
    return 0;
}

