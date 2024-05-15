#pragma once

#include <stdbool.h>

/**
 * Configuration of the game of life
*/
struct GameConfig{
    int width;  // width of the grid
    int height;  // height of the grid 
    int total_iterations;  // total iterations to simulate
    int output_steps;  // print every n-th iteration
    bool console_output; // print grid to console
    bool output_images;  // output images of the grid
    bool measure_time;  // measure time of the simulation
};
typedef struct GameConfig GameConfig;

void quitWithHelpMessage(char* name);
bool parseBool(char* value, char* name);
long parseLong(char* value, char* name, long min, long max);
GameConfig parseArguments(int argc, char** argv);
