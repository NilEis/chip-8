cmake_minimum_required(VERSION 3.23)  # Set the minimum required CMake version
project(chip-8 C)

set(CMAKE_C_STANDARD 23)  # Set the C standard to C23

# Set the paths to your source and header files
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)

add_executable(chip-8 ${SOURCES})

target_include_directories(chip-8 PUBLIC include)