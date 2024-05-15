#pragma once

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


void initializeGridZero(unsigned char** grid, int height, int width);

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

void printGrid(unsigned char** grid, int height, int width, int iteration);