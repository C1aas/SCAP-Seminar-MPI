#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#include "image_creation.h"
#include "utils_grid.h"


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

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

void debugPrint(const char *format, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}

void copyGrid(unsigned char** src_grid, unsigned char** target_grid, int height, int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            target_grid[i][j] = src_grid[i][j];
        }
    }
}

bool gridsAreEqual(unsigned char** grid_before, unsigned char** grid_after, int height, int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (grid_before[i][j] != grid_after[i][j]) {
                return false;
            }
        }
    }
    return true;
}


bool threeGridsAreEqual(unsigned char** grid1, unsigned char** grid2, unsigned char** grid3, unsigned char** grid4, int height, int width) {
    bool gridone = gridsAreEqual(grid1, grid2, height, width) || gridsAreEqual(grid1, grid3, height, width) || gridsAreEqual(grid1, grid4, height, width);
    bool gridtwo = gridsAreEqual(grid2, grid3, height, width) || gridsAreEqual(grid2, grid4, height, width);
    bool gridthree = gridsAreEqual(grid3, grid4, height, width);
    return gridone || gridtwo || gridthree;
}

void save_grid_as_image(char* name, unsigned char** grid, int height, int width, int iterations) {
    char file_name_buffer[80];
    sprintf(file_name_buffer, "grid_lifetime/%s-grid-%d-iter.jpg", name, iterations);
    write_jpeg_file(file_name_buffer, grid, width, height);
}
