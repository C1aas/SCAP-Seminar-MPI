cmake_minimum_required(VERSION 2.8)
project(GameOfLife)

# Set the C standard to C17
set(CMAKE_C_STANDARD 17)

# Ensure the C standard is strictly adhered to
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set the C compiler to scorep-mpicc
set(CMAKE_C_COMPILER scorep-gcc)

# Optimization flag
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -ljpeg")

# Add source files
set(SOURCE_FILES main.c image_creation.c )

# Set the output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Define the executable
add_executable(GameOfLife ${SOURCE_FILES})