cmake_minimum_required(VERSION 3.23) # Set the minimum required CMake version
project(chip-8 C)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

set(CMAKE_C_STANDARD 23) # Set the C standard to C23

# Set the paths to your source and header files
set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/notes.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/beep.c)

include(FetchContent)

FetchContent_Declare(
  sdl2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG origin/main)

FetchContent_MakeAvailable(sdl2)

add_executable(chip-8 ${SOURCES})

if(BUILD_SHARED_LIBS)
  target_link_libraries(chip-8 SDL3-shared)
else()
  target_link_libraries(chip-8 SDL3-static)
endif()

target_include_directories(chip-8 PUBLIC include)
