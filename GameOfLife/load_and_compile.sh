#!/bin/bash
# load_and_compile.sh

module purge

module load gcc
module load openmpi
module load scorep

# export SCOREP_ENABLE_PROFILING=true
# export SCOREP_ENABLE_TRACING=true
# export SCOREP_EXPERIMENT_DIRECTORY=scorep_output

#scorep-gcc main.c -o main.o -O2 -c 
#scorep-gcc image_creation.c -o image_creation.o -O2 -c -ljpeg 

# run CMake commands
cmake .
make
# gcc -o main main.c image_creation.c -O2 -ljpeg

echo "compilation completed."

