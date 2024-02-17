#pragma once


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