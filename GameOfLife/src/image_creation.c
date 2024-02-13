#include "image_creation.h"
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <time.h>

#define WIDTH 40000
#define HEIGHT 40000
#define DENSITY 0.3

unsigned char **createRandomBitArray(int width, int height, float density);
void freeBitArray(unsigned char **array, int height);
void printBitArray(unsigned char **bitArray, int width, int height);

/*
int main() {

    unsigned char** randomBitArray = createRandomBitArray(WIDTH, HEIGHT, DENSITY);
    // printBitArray(randomBitArray, WIDTH, HEIGHT);
    write_jpeg_file("output.jpg", randomBitArray, WIDTH, HEIGHT);

    freeBitArray(randomBitArray, HEIGHT);
    return 0;
}
*/


unsigned char **createRandomBitArray(int width, int height, float density) {
    printf("Creating Random Bit Array...");
    srand(time(NULL)); // Seed the random number generator

    // Allocate memory for the array
    unsigned char **array = malloc(height * sizeof(unsigned char *));
    if (!array) {
        fprintf(stderr, "Failed to allocate memory for the RandomBitArray\n");
        exit(1);
    }
    for (int i = 0; i < height; i++) {
        array[i] = malloc(width * sizeof(unsigned char));
        if (!array[i]) {
            fprintf(stderr, "Failed to allocate memory for the RandomBitArray row %d\n", i);
            exit(1);
        }
    }

    // Fill the array with random bits according to the specified density
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float rnd = (float)rand() / RAND_MAX;
            array[y][x] = rnd < density ? 1 : 0;
        }
    }

    return array;
}

void freeBitArray(unsigned char **array, int height) {
    if (array != NULL) {
        for (int i = 0; i < height; i++) {
            free(array[i]); // Free each row
        }
        free(array); // Free the array of pointers
    }
}

void printBitArray(unsigned char **bitArray, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            printf("%d, ", bitArray[y][x]);
        }
        printf("\n");
    }
}

void wirte1dto2dArray(unsigned char **bitArray, int height, int width, int address, int val) {
    int row = address / width; 
    int col = address % width; 
    bitArray[row][col] = val;
}

void write_jpeg_file(char *filename, unsigned char **bitArray, int width, int height) {
    // printf("Converting array to Image...\n");
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *outfile;
    JSAMPROW row_pointer[1];
    int row_stride = width;

    // Allocate memory for a temporary image where each pixel is a byte
    unsigned char *image = malloc(width * height);
    if (!image) {
        fprintf(stderr, "Failed to allocate memory for the JPEG image\n");
        exit(1);
    }

    // Convert bitArray to image
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image[y * width + x] = bitArray[y][x] ? 0 : 255; // 0 for black (bit is 1), 255 for white (bit is 0)
        }
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    if ((outfile = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        free(image);
        exit(1);
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 1; // grayscale
    cinfo.in_color_space = JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 75, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    int counter = 0;
    while (cinfo.next_scanline < cinfo.image_height) {
        if(counter % (height / 10) == 0) {
            // printf("%d/%d\n", counter, cinfo.image_height);
            fflush(stdout);
        }
        row_pointer[0] = &image[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
        counter++;
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);

    free(image);
}


