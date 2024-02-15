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

void initializeAndDistributeGrid(int height, int width);




int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int height = 20; // Example grid height
    int width = 20; // Example grid width

    initializeAndDistributeGrid(height, width);

    // Further processing, including computation and border exchange

    MPI_Finalize();
    return 0;
}

void updateGridWithLimit(WorkerConfig cfg) {
    //unsigned char** grid, int height, int width, int update_min, int update_max
    int update_min = cfg.world_rank == 0 ? 0 : 1; // if first rank, update first row
    int update_max = cfg.world_rank == cfg.world_size - 1 ? height : height - 1; // if last rank, update last row

    // First pass: Determine the next state for each cell
    for (int i = update_min; i < update_max; i++) {
        for (int j = 0; j < width; j++) {
            
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
            }
        }
    }

    

    // Second pass: Update the grid to the next state
    for (int i = update_min; i < height; i++) {
        for (int j = 0; j < width; j++) {
            grid[i][j] >>= 1; // Shift to the right to get the next state
        }
    }
    
    

}

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

void parallelGameLoop(int world_size, int world_rank, unsigned char** grid, int height, int width) {
    MPI_Barrier( MPI_COMM_WORLD );
    printf("Rank %d: Printing grid\n", world_rank);
    printGrid(grid, height, width, 0);
    
    // update the grid but dont update the border
    
    printf("Rank %d: update_min = %d, update_max = %d\n", world_rank, update_min, update_max);
    updateGridWithLimit(grid, height, width, update_min, update_max);
    printf("Rank %d: updated grid\n", world_rank);
    printGrid(grid, height, width, 0);
}

struct WorkerConfig {
    int mainRank = 0;
    int world_size;
    int world_rank;
    unsigned char** localGrid;
    int grid_height;
    int grid_width;
};
typedef struct WorkerConfig WorkerConfig;

/**
 * Initializes the worker process
 * sets the world_rank, world_size, grid_height and width
 * and receives the grid from the main process
 * 
 * @param world_rank The rank of the worker
 * @param world_size The total number of processes
 * @param grid_width The width of the grid
 * @return The configuration for the worker
 */
WorkerConfig initWorker(int world_rank, int world_size, int grid_width) {
    WorkerConfig cfg;
    MPI_Comm_rank(MPI_COMM_WORLD, &cfg.world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &cfg.world_size);
    
    // Receive the grid height
    int numRows;
    MPI_Recv(&numRows, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    cfg.grid_height = numRows;
    cfg.grid_width = grid_width;

    assert(cfg.grid_height > 0);
    assert(cfg.grid_width > 0);

    printf("Rank %d: numRows = %d\n", world_rank, numRows);
    cfg.localGrid = createGrid(numRows, width);
    for (int i = 0; i < numRows; i++) {
        printf("Rank %d: Receiving row %d\n", world_rank, i);
        MPI_Recv(cfg.localGrid[i], width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    }

    return cfg;
}

void sendandReiceiveUpdatedGrid(WorkerConfig cfg) {
    // Receive the updated grid
    
    // Send the updated grid to the neighbor processes
    MPI_Request send_request[2]; // For non-blocking sends
    int send_count = 0;

    if(cfg.world_rank > 0) {
        //send borders to the lower neighbor
        MPI_Isend(cfg.localGrid[0], cfg.grid_width, MPI_CHAR, cfg.world_rank - 1, 0, MPI_COMM_WORLD, &send_request[send_count++]);
    }

    if(cfg.world_rank < cfg.world_size - 1) {
        //send borders to the upper neighbor
        MPI_Isend(cfg.localGrid[cfg.grid_height - 1], cfg.grid_width, MPI_CHAR, cfg.world_rank + 1, 0, MPI_COMM_WORLD, &send_request[send_count++]);
    }

    
    if(cfg.world_rank > 0) {
        //receive borders from the lower neighbor
        MPI_Recv(cfg.localGrid[0], cfg.grid_width, MPI_CHAR, cfg.world_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    
    if(cfg.world_rank < cfg.world_size - 1) {
        //receive borders from the upper neighbor
        MPI_Recv(cfg.localGrid[cfg.grid_height - 1], cfg.grid_width, MPI_CHAR, cfg.world_rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Wait for non-blocking sends to complete
    if (send_count > 0) {
        MPI_Waitall(send_count, send_request, MPI_STATUSES_IGNORE);
    }


}

void startWorker() {
    // Receive the grid
    // Update the grid
    // Send the updated grid
    int numRows;
    MPI_Recv(&numRows, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    printf("Rank %d: numRows = %d\n", world_rank, numRows);
    unsigned char** localGrid = createGrid(numRows, width);
    for (int i = 0; i < numRows; i++) {
        printf("Rank %d: Receiving row %d\n", world_rank, i);
        MPI_Recv(localGrid[i], width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    }
    //printf("Rank %d: Received grid\n", world_rank);
    //printGrid(localGrid, numRows, width, 0);

    parallelGameLoop(world_size, world_rank, localGrid, numRows, width);
}

void distributeAndSendGrid(int world_size, unsigned char** grid, int height){
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
        int startRow = idx * rowsPerProcess + (idx < remainder ? idx : remainder);
        int endRow = startRow + rowsPerProcess - 1 + (idx < remainder ? 1 : 0);

        //send one line more to each process, therefor they are able to update the border
        startRow = max(startRow - 1, 0);
        endRow = min(endRow + 1, height - 1);
        
        int numRows = endRow - startRow + 1;
        printf("Rank %d : startRow = %d, endRow = %d, numRows = %d\n", rank, startRow, endRow, numRows);
        
        // Send the number of rows first
        MPI_Send(&numRows, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);

        for(int i = 0; i < numRows; i++) {
            // Send the initial grid in chunks
            printf("Rank %d -> %d: Sending row %d\n", world_rank, rank, startRow + i);
            MPI_Send(grid[startRow + i], width, MPI_CHAR, rank, 0, MPI_COMM_WORLD);
        }
        
        //MPI_Send(grid + startRow * width, numRows * width, MPI_INT, rank, 0, MPI_COMM_WORLD);
    }
}


void initializeAndDistributeGrid(int height, int width) {
    int world_rank, world_size;  

    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    


    if (world_rank == 0) {
        
        unsigned char** grid = createGrid(height, width);
    
        initializeGridRandom(grid, height, width, 0.3);
        printf("Total grid\n");
        printGrid(grid, height, width, 0);

        distributeAndSendGrid(world_size, grid, height);
        

        
        /*
        //update rank 0 grid
        printf("Rank 0: Printing grid\n");
        printGrid(grid, proc0_numRows, width, 0);


        int update_min = world_rank == 0 ? 0 : 1;
        int update_max = world_rank == proc_num ? proc0_numRows : proc0_numRows - 1; 

        updateGridWithLimit(grid, proc0_numRows, width, update_min, update_max);*/
        //parallelGameLoop(world_size, world_rank, grid, proc0_numRows, width);

    } else {
        
        WorkerConfig cfg = initWorker(world_rank, world_size, width);

        //printf("Rank %d: Received grid\n", world_rank);
        //printGrid(localGrid, numRows, width, 0);

        parallelGameLoop(cfg);

        /*
        int update_min = world_rank == 0 ? 0 : 1;
        int update_max = world_rank == proc_num ? numRows : numRows - 1; 

        updateGridWithLimit(localGrid, numRows, width, update_min, update_max);
        printf("Rank %d: updated grid\n", world_rank);
        printGrid(localGrid, numRows, width, 0);
        
        // Now, each process has its chunk and can proceed with the computation*/
    }
    // The grid (or localGrid) can now be used for computation
}
