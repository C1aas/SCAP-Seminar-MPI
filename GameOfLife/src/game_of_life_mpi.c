#include "game_of_life_mpi.h"

#include <assert.h>
#include <stdio.h>

#include "utils.h"
#include "utils_grid.h"

MPI_Datatype workerConfigType;

void createWorkerConfigMPIType(MPI_Datatype *newtype) {
    int blocklengths[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
    MPI_Datatype types[9] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint displacements[9];
    
    WorkerConfig temp;
    MPI_Aint base_address;
    MPI_Get_address(&temp, &base_address);
    MPI_Get_address(&temp.world_size, &displacements[0]);
    MPI_Get_address(&temp.world_rank, &displacements[1]);
    MPI_Get_address(&temp.total_iterations, &displacements[2]);
    MPI_Get_address(&temp.row_index_main_grid, &displacements[3]);
    
    // Ignore local_grid
    MPI_Get_address(&temp.num_rows, &displacements[4]);
    MPI_Get_address(&temp.grid_width, &displacements[5]);
    MPI_Get_address(&temp.update_start_row, &displacements[6]);
    MPI_Get_address(&temp.update_end_row, &displacements[7]);
    MPI_Get_address(&temp.update_row_count, &displacements[8]);

    // Korrektur der Displacements
    for (int i = 0; i < 9; i++) {
        displacements[i] -= base_address;
    }

    MPI_Type_create_struct(9, blocklengths, displacements, types, newtype);
    MPI_Type_commit(newtype);
}


WorkerConfig initWorkerConfig(int world_size, int world_rank, int row_index_main_grid, int num_rows, int grid_width, int start_row, int total_iterations) {
    assert(grid_width > 0);
    assert(num_rows > 0);
    WorkerConfig cfg = {
        .world_size = world_size,
        .world_rank = world_rank,
        .total_iterations = total_iterations,
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


void distributeAndSendConfig(int world_size, int height, int width, int total_iterations, WorkerConfig* workerConfigs){
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
        WorkerConfig cfg = initWorkerConfig(world_size, rank, row_index_main_grid, num_rows, width, start_row, total_iterations);
        workerConfigs[idx] = cfg;
        // send the config to the worker
        //printf("main -> %d: Sending config\n", rank);
        MPI_Send(&cfg, 1, workerConfigType, cfg.world_rank, 0, MPI_COMM_WORLD);
        //printf("main -> %d: Sent config\n", cfg.world_rank);

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

    debugPrint("main: Sent all Grids\n");
}


WorkerConfig receiveWorkerConfig() {
    WorkerConfig cfg;
    MPI_Recv(&cfg, 1, workerConfigType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //print the config for debugging
    //printf("Rank %d: Received config: world_size: %d, world_rank: %d, row_index_main_grid: %d, num_rows: %d, grid_width: %d, update_start_row: %d, update_end_row: %d, update_row_count: %d\n", cfg.world_rank, cfg.world_size, cfg.world_rank, cfg.row_index_main_grid, cfg.num_rows, cfg.grid_width, cfg.update_start_row, cfg.update_end_row, cfg.update_row_count);
    return cfg;
}


void receiveInitialGrid(WorkerConfig* cfg) {
    // printf("Rank %d: receiving %d rows\n", cfg->world_rank, cfg->num_rows);
    cfg->local_grid = createGrid(cfg->num_rows, cfg->grid_width);
    for (int i = 0; i < cfg->num_rows; i++) {
        //printf("Rank %d: Receiving row %d\n", world_rank, i);
        MPI_Recv(cfg->local_grid[i], cfg->grid_width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

}


void updateGridWithLimit(WorkerConfig cfg) {
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


void receiveGridParts(WorkerConfig* workerConfigs, unsigned char** grid, int height, int width, int world_size) {
    printf("Main: waiting to receive total grid from workers\n");
    MPI_Request requests[height]; // send all grid rows
    //printf("Main: waiting to receive a total of %d rows\n", height);
    int request_count = 0;
    for (int worker_idx = 1; worker_idx < world_size; worker_idx++) {
        
        WorkerConfig* cfg = &workerConfigs[worker_idx-1];
        //printf("Main: waiting to receive %d rows from %d\n", cfg->update_row_count, worker_idx);
        for(int i = 0; i < cfg->update_row_count; i++) {
            //MPI_Recv(grid[cfg->row_index_main_grid + i], width, MPI_CHAR, worker_idx, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //printf("Main: Received row %d from %d\n", cfg->row_index_main_grid + i, worker_idx);
            MPI_Irecv(grid[cfg->row_index_main_grid + i], width, MPI_CHAR, worker_idx, 0, MPI_COMM_WORLD, &requests[request_count++]);
        }
    }
    //printf("Request count: %d height: %d\n", request_count, height);
    MPI_Waitall(request_count, requests, MPI_STATUSES_IGNORE); // wait for all rows to be received
    
    //printf("Main: Received total grid\n");
}