cmake_minimum_required(VERSION 3.2) # Set the minimum required CMake version
project(chip-8 C)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

set(CMAKE_C_STANDARD 17)
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/programs/1-chip8-logo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/programs/2-ibm-logo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/video/screen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/notes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/beep.c)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

file(DOWNLOAD
     https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
     ${CMAKE_BINARY_DIR}/CPM.cmake)
include(${CMAKE_BINARY_DIR}/CPM.cmake)

option(FILE_OPEN "use file opener" ON)

cpmaddpackage("gh:libsdl-org/SDL#a509771a87559ca040a40b5fef932a8220343f7a")

if(FILE_OPEN)
  add_compile_definitions(FILE_OPEN=1)
  cpmaddpackage("gh:btzy/nativefiledialog-extended#master")
endif()

add_executable(chip-8 ${SOURCES})

if(BUILD_SHARED_LIBS)
  target_link_libraries(chip-8 SDL3-shared Threads::Threads
                        $<$<BOOL:${FILE_OPEN}>:nfd>)
else()
  target_link_libraries(chip-8 SDL3-static Threads::Threads
                        $<$<BOOL:${FILE_OPEN}>:nfd>)
endif()

target_include_directories(chip-8 PUBLIC include)
