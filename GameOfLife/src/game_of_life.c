#include "game_of_life.h"

#include <time.h> // time
#include <stdio.h> // printf
#include <stdlib.h> // malloc, free, rand
#include <unistd.h> // usleep

#include "image_creation.h" // write_jpeg_file


unsigned char** createGrid(int height, int width) {
    unsigned char** grid = malloc(height * sizeof(unsigned char *));
    for (int i = 0; i < height; i++) {
        grid[i] = malloc(width * sizeof(unsigned char));
    }
    return grid;
}

unsigned char* createGridSingleBlock(int height, int width) {
    unsigned char* grid = malloc(height * width * sizeof(unsigned char *));
    if (grid == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    return grid;
}

void freeGrid(unsigned char** grid, int height) {
    for (int i = 0; i < height; i++) {
        free(grid[i]);
    }
    free(grid);
}

void initializeGridRandom(unsigned char** grid, int height, int width, float density) {
    srand(time(NULL)); // Seed the random number generator with current time
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float probability = rand() / (RAND_MAX + 1.0);
            grid[i][j] = probability < density ? 1 : 0;
        }
    }
}

void initializeGridRandom1D(unsigned char* grid, int height, int width, float density) {
    srand(time(NULL)); // Seed the random number generator with current time
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float probability = rand() / (RAND_MAX + 1.0);
            grid[i * height + j] = probability < density ? 1 : 0;
        }
    }
}

void initializeGridModulo(unsigned char** grid, int height, int width, int modulo) {    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            grid[i][j] = i % 3 == 0 ? 1 : 0;
        }
    }
}

void initializeGridSpaceCraft(unsigned char** grid, int height, int width, int offsety, int offsetx) {
    int oy = offsety;
    int ox = offsetx;

    grid[oy + 0][ox + 1] = 1;
    grid[oy + 0][ox + 2] = 1;

    grid[oy + 1][ox + 0] = 1;
    grid[oy + 1][ox + 1] = 1;
    grid[oy + 1][ox + 3] = 1;
    grid[oy + 1][ox + 4] = 1;
    grid[oy + 1][ox + 5] = 1;
    grid[oy + 1][ox + 6] = 1;

    grid[oy + 2][ox + 1] = 1;
    grid[oy + 2][ox + 2] = 1;
    grid[oy + 2][ox + 3] = 1;
    grid[oy + 2][ox + 4] = 1;
    grid[oy + 2][ox + 5] = 1;
    grid[oy + 2][ox + 6] = 1;

    grid[oy + 3][ox + 2] = 1;
    grid[oy + 3][ox + 3] = 1;
    grid[oy + 3][ox + 4] = 1;
    grid[oy + 3][ox + 5] = 1;
}

void updateGrid(unsigned char** grid, int height, int width) {
    // First pass: Determine the next state for each cell
    for (int i = 0; i < height; i++) {
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
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            grid[i][j] >>= 1; // Shift to the right to get the next state
        }
    }
}

void printGrid(unsigned char** grid, int height, int width, int iteration) {
    // Print top border
    for (int j = 0; j < width*2 + 2; j++) printf("-");
    printf("\n");

    for (int i = 0; i < height; i++) {
        printf("|"); // Left border
        for (int j = 0; j < width; j++) {
            //printf("%c ", grid[i][j] ? 'O' : '.');
            printf("%d ", (int)grid[i][j]);
        }
        printf("|\n"); // Right border
    }

    // Print bottom border
    for (int j = 0; j < width*2 + 2; j++) printf("-");
    printf("\n");
    printf("iteration: %d \n", iteration);
}

void run(GameConfig cfg) {
    printf("Starting game of life simulation");
    if (cfg.measure_time) {
        printf(" with time measurement\n");
        measuredGameLoop(cfg);
    } else {
        printf(" without time measurement\n");
        gameLoop(cfg);
    }
}

void gameLoop(GameConfig cfg) {
    int height = cfg.grid_size;
    int width = cfg.grid_size;

    unsigned char** grid = createGrid(height, width);
    initializeGridModulo(grid, height, width, 3);
    //initializeGridSpaceCraft(grid, height, width, 20, 20);

    int iteration = 0;
    char file_name_buffer[80];

    while (1) {
        
        if(cfg.output_steps > 0) {
            if(iteration % cfg.output_steps == 0) {
                sprintf(file_name_buffer, "output_images/GoLOutput%d.jpg", iteration);
                
                if (cfg.console_output) {
                    //system("clear"); // Clear the console
                    printGrid(grid, height, width, iteration);
                    //usleep(200000); // Sleep for 200 milliseconds
                    usleep(1000000); // Sleep for 200 milliseconds
                    //usleep(20000); // Sleep for 20 milliseconds
                }

                if (cfg.output_images) {
                    printf("Writing image to '%s'\n", file_name_buffer);
                    write_jpeg_file(file_name_buffer, grid, width, height);
                }
            }
        }

        if(iteration >= cfg.total_iterations) {
            printf("Done.\n");
            break;
        }

        //updateGridOptimized(grid, height, width);
        updateGrid(grid, height, width);
        //usleep(200000); // Sleep for 200 milliseconds
        // usleep(20000); // Sleep for 20 milliseconds
        // usleep(2000); // Sleep for 2 milliseconds
        
        iteration++;
    }

    freeGrid(grid, height);

}


/**
 * Helper function to get the current time in milliseconds
*/
long long current_time_millis() {
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    long long millis = spec.tv_sec * 1000LL + spec.tv_nsec / 1000000; // Convert seconds and nanoseconds to milliseconds
    return millis;
}

void measuredGameLoop(GameConfig cfg) {
    long long start = current_time_millis();

    gameLoop(cfg);

    long long end = current_time_millis();
    long long total = end - start;

    printf("\n----------------------------------------\n");
    printf("Time taken: %lld milliseconds\n", total);
    printf("Time per iteration: %lld milliseconds\n", total / cfg.total_iterations);

    if (cfg.grid_size > 1000) {
        printf("Total of %d of 1000x1000 subgrids\n", (cfg.grid_size*cfg.grid_size)/(1000*1000));
        printf("Time taken per 1000x1000: %lld milliseconds\n", total /((cfg.grid_size*cfg.grid_size)/(1000*1000)));
    }

    printf("----------------------------------------\n");
}
