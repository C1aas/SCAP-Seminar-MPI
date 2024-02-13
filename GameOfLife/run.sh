#!/bin/bash
#SBATCH -N 1
##SBATCH --ntasks 1
#SBATCH --tasks-per-node=1
#SBATCH --job-name parallel_hello
#SBATCH --partition medium
#SBATCH --time 0:05:00
#SBATCH --output slurm_output/job_output_%x_%j_$(date +%Y-%m-%d_%H-%M-%S).txt

##SBATCH--exclusive

#module purge

#module load gcc

#export SCOREP_ENABLE_PROFILING=true
#export SCOREP_ENABLE_TRACING=true
# gcc main.c -o main -O2
module purge
module load openmpi

export SCOREP_ENABLE_PROFILING=true
export SCOREP_ENABLE_TRACING=true

#export SCOREP_EXPERIMENT_DIRECTORY=scorep_output


# mpiexec -np 1 time ./main 1000 10 -1

./bin/GameOfLife 50 50 1 true false true

# ./main 100 100 10 true false true