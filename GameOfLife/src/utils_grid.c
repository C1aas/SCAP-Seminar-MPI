#include <stdio.h>
#include <stdlib.h> // malloc, free, rand
#include <time.h>

#include "utils_grid.h"

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

void initializeGridZero(unsigned char** grid, int height, int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            grid[i][j] = 0;
        }
    }

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

void printGrid(unsigned char** grid, int height, int width, int iteration) {
    // Print top border
    for (int j = 0; j < width*2 + 2; j++) printf("-");
    printf("\n");

    for (int i = 0; i < height; i++) {
        printf("|"); // Left border
        for (int j = 0; j < width; j++) {
            printf("%c ", grid[i][j] ? 'O' : '.');
            //printf("%d ", (int)grid[i][j]);
        }
        printf("|\n"); // Right border
    }

    // Print bottom border
    for (int j = 0; j < width*2 + 2; j++) printf("-");
    printf("\n");
    printf("iteration: %d \n", iteration);
}
