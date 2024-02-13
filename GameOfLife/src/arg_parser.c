#include "arg_parser.h"

#include <stdio.h> // printf
#include <stdlib.h> // exit
#include <string.h> // strcmp

void quitWithHelpMessage(char* name) {
    printf("Usage: %s <grid_size:int> <total_iterations:int> <output_steps:int> <console_output:bool> <output_images:bool> <measure_time:bool>\n", name);
    exit(1);
}

bool parseBool(char* value, char* name) {
    if (strcmp(value, "true") == 0) {
        return true;
    } else if (strcmp(value, "false") == 0) {
        return false;
    } else {
        printf("Ivalid bool value for %s\n", name);
        exit(1);
    }
}

long parseLong(char* value, char* name, long min, long max) {
    int result = strtol(value, NULL, 0);
    if (result < min || result > max) {
        printf("Invalid value for %s, needs to be between %ld and %ld\n", name, min, max);
        exit(1);
    }
    return result;
}

GameConfig parseArguments(int argc, char** argv) {
    GameConfig cfg;
    if(argc != 7) {
        printf("Error: Invalid number of arguments. %d were given and 6 are expected\n", argc-1);
        quitWithHelpMessage(argv[0]);
    }
    //check for help flag
    if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("This program simulates the game of life\n");
        quitWithHelpMessage(argv[0]);
    }

    cfg.grid_size = parseLong(argv[1], "grid_size", 10, 40000);
    cfg.total_iterations = parseLong(argv[2], "total_iterations", 1, 1000000);
    cfg.output_steps = parseLong(argv[3], "output_steps", -1, 1000000);


    //parse the true/false arguments
    cfg.console_output = parseBool(argv[4], "console_output");
    if (cfg.console_output && cfg.grid_size > 100) {
        printf("Warning: This setting will generate very large console output\n");
        //exit(1);
    }
    cfg.output_images = parseBool(argv[5], "output_images");
    cfg.measure_time = parseBool(argv[6], "measure_time");
   


    if(cfg.output_images && cfg.total_iterations / cfg.output_steps > 10) {
        printf("Error: This setting would generate %d output Images. Maximum is 10\n", cfg.total_iterations / cfg.output_steps);
        exit(1);
    }

    //print title and parsed arguments
    printf("***************************\n");
    printf("* Game of Life Simulation *\n");
    printf("***************************\n");
    printf("size: %d x %d; total_iterations: %d; output steps: %d; console_output: %s; output images: %s; measure time: %s\n", cfg.grid_size, cfg.grid_size, cfg.total_iterations, cfg.output_steps, cfg.console_output ? "true" : "false", cfg.output_images ? "true" : "false", cfg.measure_time ? "true" : "false");
    printf("\n");
    return cfg;
}