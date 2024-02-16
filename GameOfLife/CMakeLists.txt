cmake_minimum_required(VERSION 3.1)
project(GameOfLife)

# Set the include directories, this is where the header files are located
include_directories(${PROJECT_SOURCE_DIR}/src)

# Set the C standard to C17
set(CMAKE_C_STANDARD 17)

# Ensure the C standard is strictly adhered to
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set the C compiler to scorep-mpicc
set(CMAKE_C_COMPILER mpicc)

# Optimization flag
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -ljpeg")

# Add source files
set(SOURCE_FILES src/main_mpi.c src/game_of_life.c src/arg_parser.c src/image_creation.c )

# Set the output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Define the executable
add_executable(GameOfLife ${SOURCE_FILES})
