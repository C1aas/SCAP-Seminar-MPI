#include <stdio.h>
#include <mpi.h>

#include "game_of_life.h"
#include "arg_parser.h"

/*
Optimizations
- Use a 2 bit to store the next state of the cell

*/

int main(int argc, char** argv) {
    GameConfig cfg = parseArguments(argc, argv);
    run(cfg);
    return 0;
}