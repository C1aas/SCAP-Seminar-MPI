#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#include "game_of_life.h"

void writeGridToFile(char* name, unsigned char** grid, int height, int width, int iterations) {
    FILE *filePointer;
    char file_name_buffer[80];
    
    sprintf(file_name_buffer, "grid_lifetime/%s-grid-%d-iter.txt", name, iterations);
    filePointer = fopen(file_name_buffer, "w");

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            fprintf(filePointer, "%c", grid[i][j] ? '1' : '0');
        }
        fprintf(filePointer, "\n");
    }
    fclose(filePointer);
}

unsigned char** readGridFromFile(char* name, int height, int width, int iterations) {
    FILE *filePointer;
    char file_name_buffer[80];
    
    sprintf(file_name_buffer, "grid_lifetime/%s-grid-%d-iter.txt", name, iterations);
    filePointer = fopen(file_name_buffer, "r");

    unsigned char** grid = createGrid(height, width);
    for (int i = 0; i < height; i++) {
        char line[width];
        fgets(line, width + 1, filePointer);
        for (int j = 0; j < width; j++) {
            grid[i][j] = line[j] == '1' ? 1 : 0;
        }
    }
    fclose(filePointer);
    return grid;
}

void printRow(int rank, unsigned char* row, int width) {
    printf("Rank %d: ", rank);
    for (int i = 0; i < width; i++) {
        printf("%d", row[i]);
    }
    printf("\n");
}