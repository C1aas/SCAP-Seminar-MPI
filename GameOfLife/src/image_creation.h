#pragma once

/**
 * Writes a jpeg file with the given bitArray
 * 
 * @param filename The name of the file to be written
 * @param bitArray The array of pixels to be written
 * @param width The width of the bitArray and thefor the image
 * @param height The height of the bitArray and thefor the image
 */
void write_jpeg_file(char *filename, unsigned char **bitArray, int width, int height);

