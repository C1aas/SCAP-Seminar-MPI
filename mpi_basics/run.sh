#!/bin/bash
#SBATCH -N 1
##SBATCH --ntasks 4
#SBATCH --tasks-per-node=4
#SBATCH --job-name parallel_hello
#SBATCH --partition medium
#SBATCH --time 0:01:00
#SBATCH --output parallel_hello_world.out



# Check if an executable name is provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <executable_name>"
    exit 1
fi

module purge

module load gcc
module load openmpi

# Execute the program with the provided name

mpiexec -np 4 ./"$1"
