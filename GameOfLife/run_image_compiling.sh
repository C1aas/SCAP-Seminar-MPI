#!/bin/bash
#SBATCH -N 1
##SBATCH --ntasks 1
#SBATCH --tasks-per-node=1
#SBATCH --job-name parallel_hello
#SBATCH --partition fat
#SBATCH --time 0:05:00
#SBATCH --output image.out

##SBATCH--exclusive

#module purge

#module load gcc

#export SCOREP_ENABLE_PROFILING=true
#export SCOREP_ENABLE_TRACING=true
# gcc main.c -o main -O2

time ./image_creation
