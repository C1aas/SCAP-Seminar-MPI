#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>


#include "image_creation.h"

#define DENSITY 0.3

/*
Optimizations
- Use a 2 bit to store the next state of the cell

*/

typedef struct Arguments {
    int grid_size;  // size of the grid size x size
    int total_iterations;  // total iterations to simulate
    int output_steps;  // print every n-th iteration
    bool console_output;
    bool output_images;  // output images
    bool measure_time;  // measure time
}Arguments;


unsigned char** createGrid(int height, int width);

void freeGrid(unsigned char** grid, int height);

void initializeGridRandom(unsigned char** grid, int height, int width, float density);
void initializeGridModulo(unsigned char** grid, int height, int width, int modulo);
void initializeGridSpaceCraft(unsigned char** grid, int height, int width, int offsety, int offsetx);

void updateGrid(unsigned char** grid, int height, int width);
void updateGridOptimized(unsigned char**, int height, int width);

void printGrid(unsigned char**, int height, int width, int iteration);
void printGridFlat(unsigned char**, int height, int width, int iteration);

int writeGridToFile(FILE* file, unsigned char**, int height, int width, int iteration);

void gameLoop(Arguments args);
void measuredGameLoop(Arguments args);

FILE* initOutputFile(int height, int width, int max_iterations, int output_steps);

void quitWithHelpMessage(char* name) {
    printf("Usage: %s <grid_size:int> <total_iterations:int> <output_steps:int> <console_output:bool> <output_images:bool> <measure_time:bool>\n", name);
    exit(1);
}

bool parseBool(char* value, char* name) {
    if (strcmp(value, "true") == 0) {
        return true;
    } else if (strcmp(value, "false") == 0) {
        return false;
    } else {
        printf("Ivalid bool value for %s\n", name);
        exit(1);
    }
}

long parseLong(char* value, char* name, long min, long max) {
    int result = strtol(value, NULL, 0);
    if (result < min || result > max) {
        printf("Invalid value for %s, needs to be between %ld and %ld\n", name, min, max);
        exit(1);
    }
    return result;
}

Arguments parseArguments(int argc, char** argv) {
    Arguments args;
    if(argc != 7) {
        printf("Error: Invalid number of arguments. %d were given and 6 are expected\n", argc-1);
        quitWithHelpMessage(argv[0]);
    }
    //check for help flag
    if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("This program simulates the game of life\n");
        quitWithHelpMessage(argv[0]);
    }

    args.grid_size = parseLong(argv[1], "grid_size", 10, 40000);
    args.total_iterations = parseLong(argv[2], "total_iterations", 1, 1000000);
    args.output_steps = parseLong(argv[3], "output_steps", -1, 1000000);

    
    //parse the true/false arguments

    args.console_output = parseBool(argv[4], "console_output");
    if (args.grid_size > 100) {
        printf("Error: This setting would very large console output\n");
        exit(1);
    }
    args.output_images = parseBool(argv[5], "output_images");
    args.measure_time = parseBool(argv[6], "measure_time");
   


    if(args.output_images && args.total_iterations / args.output_steps > 10) {
        printf("Error: This setting would generate %d output Images. Maximum is 10\n", args.total_iterations / args.output_steps);
        exit(1);
    }

    //print title and parsed arguments
    printf("***************************\n");
    printf("* Game of Life Simulation *\n");
    printf("***************************\n");
    printf("size: %d x %d; total_iterations: %d; output steps: %d; console_output: %s; output images: %s; measure time: %s\n", args.grid_size, args.grid_size, args.total_iterations, args.output_steps, args.console_output ? "true" : "false", args.output_images ? "true" : "false", args.measure_time ? "true" : "false");
    printf("\n");
    return args;
}

int main(int argc, char** argv) {
    Arguments args;
    args = parseArguments(argc, argv);

    printf("Starting game of life simulation");
    if (args.measure_time) {
        printf(" with time measurement\n");
        measuredGameLoop(args);
    } else {
        printf(" without time measurement\n");
        gameLoop(args);
    }

    return 0;
}

long long current_time_millis() {
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    long long millis = spec.tv_sec * 1000LL + spec.tv_nsec / 1000000; // Convert seconds and nanoseconds to milliseconds
    return millis;
}

void measuredGameLoop(Arguments args) {
    long long start = current_time_millis();

    gameLoop(args);

    long long end = current_time_millis();
    long long total = end - start;
    printf("\n----------------------------------------\n");
    printf("Time taken: %lld milliseconds\n", total);
    printf("Time per iteration: %lld milliseconds\n", total / args.total_iterations);

    if (args.grid_size > 1000) {
        printf("Total of %d of 1000x1000 subgrids\n", (args.grid_size*args.grid_size)/(1000*1000));
        printf("Time taken per 1000x1000: %lld milliseconds\n", total /((args.grid_size*args.grid_size)/(1000*1000)));
    }

    printf("----------------------------------------\n");
}



void gameLoop(Arguments args) {
    
    /* old output logic
    FILE* filePointer = NULL;
    if(output_steps > 0) {
        FILE* filePointer = initOutputFile(height, width, max_iterations, output_steps);
    }
    */
    int height = args.grid_size;
    int width = args.grid_size;


    unsigned char** grid = createGrid(height, width);
    initializeGridModulo(grid, height, width, 3);
    //initializeGridSpaceCraft(grid, height, width, 20, 20);

    int iteration = 0;
    char file_name_buffer[80];

    while (1) {
        
        if(iteration % args.output_steps == 0) {
            // 
            if(args.output_steps > 0) {
                //writeGridToFile(filePointer, grid, height, width, iteration);
                sprintf(file_name_buffer, "output_images/GoLOutput%d.jpg", iteration);
                
                if (args.console_output) {
                    system("clear"); // Clear the console
                    printGrid(grid, height, width, iteration);
                    usleep(200000); // Sleep for 200 milliseconds
                }

                if (args.output_images) {
                    printf("Writing image to '%s'\n", file_name_buffer);
                    write_jpeg_file(file_name_buffer, grid, width, height);
                }
            }
        }

        if(iteration >= args.total_iterations) {
            printf("Done.\n");
            break;
        }

        updateGridOptimized(grid, height, width);
        //updateGrid(grid, height, width);
        //usleep(200000); // Sleep for 200 milliseconds
        // usleep(20000); // Sleep for 20 milliseconds
        // usleep(2000); // Sleep for 2 milliseconds
        
        iteration++;
    }

    /* old output logic
    if(output_steps > 0) { 
        // Close the file to save changes
        fclose(filePointer);
    }*/

    freeGrid(grid, height);

}

unsigned char** createGrid(int height, int width) {
    unsigned char** grid = malloc(height * sizeof(unsigned char *));
    for (int i = 0; i < height; i++) {
        grid[i] = malloc(width * sizeof(unsigned char));
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

void printGrid(unsigned char** grid, int height, int width, int iteration) {
    // Print top border
    for (int j = 0; j < width + 2; j++) printf("-");
    printf("\n");

    for (int i = 0; i < height; i++) {
        printf("|"); // Left border
        for (int j = 0; j < width; j++) {
            printf("%c", grid[i][j] ? 'O' : '.');
        }
        printf("|\n"); // Right border
    }

    // Print bottom border
    for (int j = 0; j < width + 2; j++) printf("-");
    printf("\n");
    printf("iteration: %d \n", iteration);
}


void updateGrid(unsigned char** grid, int height, int width) {
    // Allocate a temporary grid to hold the next state
    unsigned char** tempGrid = createGrid(height, width);
    if (tempGrid == NULL) {
        printf("Memory allocation failed for tempGrid\n");
        return; // Exit if memory allocation fails
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int liveNeighbors = 0;

            // Check all eight neighbors
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    if (y == 0 && x == 0) continue; // Skip the cell itself

                    int ni = i + y;
                    int nj = j + x;

                    // Check for edge cases
                    if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                        liveNeighbors += grid[ni][nj];
                    }
                }
            }

            // Apply the rules of the game
            if (grid[i][j] == 1 && (liveNeighbors < 2 || liveNeighbors > 3)) {
                tempGrid[i][j] = 0; // Die
            } else if (grid[i][j] == 0 && liveNeighbors == 3) {
                tempGrid[i][j] = 1; // Reproduce
            } else {
                tempGrid[i][j] = grid[i][j]; // Stay the same
            }
        }
    }

    // Copy the next state back to the original grid and free tempGrid
    for (int i = 0; i < height; i++) {
        memcpy(grid[i], tempGrid[i], width * sizeof(int));
    }
    freeGrid(tempGrid, height);
}

void updateGridOptimized(unsigned char** grid, int height, int width) {
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
            //printf("live neighbors: %d\n", liveNeighbors);

            // Apply the Game of Life rules using the additional bits for the next state
            if (grid[i][j] & 1) { // Currently alive, correctly check using & 1
                if (liveNeighbors == 2 || liveNeighbors == 3) { // otherwise over and under population death
                    // a cell is alive 'x1' and next round it needs to live set 2nd bit to 1 '11'
                    grid[i][j] |= 2; // Set the second bit to stay alive in the next state
                }
            } else { // Currently dead
                if (liveNeighbors == 3) {
                    // a cell is dead 'x0' and next round it needs to be born set 2nd bit to 1 '10'
                    grid[i][j] |= 2; // Dead cell becoming alive, set the second bit
                }
            }
        }
    }

    // Second pass: Update the grid to the next state
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            //printf("(%d/%d) alive: '%d' -> '%d' \n", i, j, grid[i][j], grid[i][j] >> 1);
            grid[i][j] >>= 1; // Shift to the right to get the next state
        }
    }
}

FILE* initOutputFile(int height, int width, int max_iterations, int output_steps) {
    FILE *filePointer;
    
    // Open the file "example.txt" in write mode
    filePointer = fopen("gridOutput.txt", "w");
    
    // Check if the file was opened successfully
    if (filePointer == NULL) {
        printf("Error opening the file.\n");
        return NULL; // Return an error code
    }
    
    // Write text to the file
    //char buffer[100];
    //snprintf(buffer, 100, "size (H, W): %d x %d\nmax iterations: %d\noutput steps: %d\n\n", height, width, max_iterations, output_steps)
    
    fprintf(filePointer, "size (H, W): %d x %d\n", height, width);
    fprintf(filePointer, "max iterations: %d\n", max_iterations);
    fprintf(filePointer, "output steps: %d\n\n", output_steps);

    return filePointer;
}

int writeGridToFile(FILE* filePointer, unsigned char** grid, int height, int width, int iteration) {
    fprintf(filePointer, "%d\n", iteration);  //write iteration to file
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            fprintf(filePointer, "%c", grid[i][j] ? '1' : '0');
            /*
            if(j+1 < width) {
                fprintf(filePointer, ",");
            }*/
        }
        fprintf(filePointer, "\n");
    }
    fprintf(filePointer, "\n");  //double newline at the end
}