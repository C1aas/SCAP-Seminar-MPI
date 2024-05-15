#!/bin/bash
#SBATCH -N 1
#SBATCH --ntasks 11
##SBATCH --tasks-per-node=1
#SBATCH --job-name=GoLSim
#SBATCH --partition medium
#SBATCH --time 0:02:00
#SBATCH --output slurm_output/GoL_job_output.txt

##SBATCH--exclusive

#module purge

#module load gcc

#export SCOREP_ENABLE_PROFILING=true
#export SCOREP_ENABLE_TRACING=true
# gcc main.c -o main -O2
module purge
module load openmpi

# export SCOREP_EXPERIMENT_DIRECTORY=scorep_output/scorep

export SCOREP_ENABLE_PROFILING=true
export SCOREP_ENABLE_TRACING=true

export SCOREP_FILTERING_FILE=GoL-scorep.filt


#export SCOREP_MEMORY_RECORDING=true

# Level 1 and 2 data cache misses, doesnt work, papi not installed
export SCOREP_METRIC_PAPI=PAPI_L1_DCM,PAPI_L2_DCM
#,PAPI_L3_DCM


# mpiexec -np 1 time ./main 1000 10 -1

# <grid_size:int> <total_iterations:int> <output_steps:int> <console_output:bool> <output_images:bool> <measure_time:bool>
#time ./bin/GameOfLife 100 50 5 false true false

#for i in {1..200}
#do
#   mpiexec -np 11 ./bin/GameOfLife 1000 100 10 false false false
#done
#mpiexec -np 11 ./bin/GameOfLife

#module load cube
#module load scalasca


mpirun -np 11 ./bin/GameOfLife 10000 100 10 false false false
# scalasca -analyze -s mpiexec -np 11 ./bin/GameOfLife
