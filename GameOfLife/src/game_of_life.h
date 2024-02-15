#pragma once
#include <stdbool.h>

#define DENSITY 0.3 // density of black cells in the initial grid

/**
 * Configuration of the game of life
*/
struct GameConfig{
    int grid_size;  // size of the grid size x size
    int total_iterations;  // total iterations to simulate
    int output_steps;  // print every n-th iteration
    bool console_output; // print grid to console
    bool output_images;  // output images of the grid
    bool measure_time;  // measure time of the simulation
};
typedef struct GameConfig GameConfig;

/**
 * Create a grid with the given height and width the memory block is allocated row by row
 * 
 * @param height the height of the grid
 * @param width the width of the grid
 * @return the created grid
*/
unsigned char** createGrid(int height, int width);

/**
 * Create a grid with the given height and width the memory block is allocated in a single block
 * 
 * @param height the height of the grid
 * @param width the width of the grid
 * @return the created grid
*/
unsigned char* createGridSingleBlock(int height, int width);

/**
 * Free the memory of the grid
 * 
 * @param grid the grid to free
 * @param height the height of the grid
*/
void freeGrid(unsigned char** grid, int height);

/**
 * Initialize the grid with random pixel of black and white with the given density
 * 
 * @param grid the grid to initialize
 * @param height the height of the grid
 * @param width the width of the grid
 * @param density the density of black cells in the grid
*/
void initializeGridRandom(unsigned char** grid, int height, int width, float density);

void initializeGridRandom1D(unsigned char* grid, int height, int width, float density);

void initializeGridModulo(unsigned char** grid, int height, int width, int modulo);

void initializeGridSpaceCraft(unsigned char** grid, int height, int width, int offsety, int offsetx);

void updateGrid(unsigned char** grid, int height, int width);

void printGrid(unsigned char**, int height, int width, int iteration);

/**
 * Start the game of life simulation with the given configuration
 * either with or without time measurement
 * 
 * @param cfg the configuration of the game
*/
void run(GameConfig cfg);

/**
 * Run the game of life simulation with the given configuration
 * 
 * @param cfg the configuration of the game
*/
void gameLoop(GameConfig cfg);

/**
 * Run the game of life simulation with the given configuration and measure the time
 * 
 * @param cfg the configuration of the game
*/
void measuredGameLoop(GameConfig cfg);