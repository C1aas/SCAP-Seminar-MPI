unsigned char **createRandomBitArray(int width, int height, float density);
void write_jpeg_file(char *filename, unsigned char **bitArray, int width, int height);
void freeBitArray(unsigned char **array, int height);
void printBitArray(unsigned char **bitArray, int width, int height);
