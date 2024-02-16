#!/bin/bash
#SBATCH -N 1
#SBATCH --ntasks 21
##SBATCH --tasks-per-node=1
#SBATCH --job-name parallel_hello
#SBATCH --partition medium
#SBATCH --time 0:15:00
#SBATCH --output slurm_output/GoL_job_output.txt

##SBATCH--exclusive

#module purge

#module load gcc

#export SCOREP_ENABLE_PROFILING=true
#export SCOREP_ENABLE_TRACING=true
# gcc main.c -o main -O2
module purge
module load openmpi

export SCOREP_EXPERIMENT_DIRECTORY=scorep_output/scorep

export SCOREP_ENABLE_PROFILING=true
export SCOREP_ENABLE_TRACING=true

export SCOREP_MEMORY_RECORDING=true

# Level 1 and 2 data cache misses
export SCOREP_METRIC_PAPI=PAPI_L1_DCM,PAPI_L2_D




# mpiexec -np 1 time ./main 1000 10 -1

# <grid_size:int> <total_iterations:int> <output_steps:int> <console_output:bool> <output_images:bool> <measure_time:bool>
# time ./bin/GameOfLife 1000 50 -1 false false true
mpiexec -np 21 ./bin/GameOfLife
