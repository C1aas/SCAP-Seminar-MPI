module purge

module load gcc
module load openmpi


# Check if at least one argument is provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <source_file.c> [-p]"
    exit 1
fi

# Extract the base name of the C file (without the .c extension)
base_name=$(basename -s .c "$1")

# Check if the second argument is -p for profiling
if [ "$2" = "-p" ]; then

    echo "Compiling with scoreP."
    module load scorep
    export SCOREP_ENABLE_PROFILING=true
    export SCOREP_ENABLE_TRACING=true

    scorep-mpicc "$1" -o "${base_name}-scorep"

else
    echo "Normal compilation mode."
    mpicc "$1" -o "${base_name}"

fi


echo "Compiling done."