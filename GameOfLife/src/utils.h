#pragma once

#include <stdbool.h>

void writeGridToFile(char* name, unsigned char** grid, int height, int width, int iterations);

unsigned char** readGridFromFile(char* name, int height, int width, int iterations);

/**
 * Print the grid to the console
 * 
 * @param grid The grid to print
 * @param height The height of the grid
 * @param width The width of the grid
 */
void printRow(int rank, unsigned char* row, int width);


int min(int a, int b);
int max(int a, int b);

// #define DEBUG

void debugPrint(const char *format, ...);

void copyGrid(unsigned char** src_grid, unsigned char** target_grid, int height, int width);

bool gridsAreEqual(unsigned char** grid_before, unsigned char** grid_after, int height, int width);

/**
 * Checks if 3 grids are equal
 * 
 * @param grid1 The first grid
 * @param grid2 The second grid
 * @param grid3 The third grid
 * @param grid4 The fourth grid
 * @param height The height of the grid
 * @param width The width of the grid
 * @return True if the simulation has stopped, false otherwise
*/
bool threeGridsAreEqual(unsigned char** grid1, unsigned char** grid2, unsigned char** grid3, unsigned char** grid4, int height, int width);

void save_grid_as_image(char* name, unsigned char** grid, int height, int width, int iterations);