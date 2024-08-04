cmake_minimum_required(VERSION 3.2) # Set the minimum required CMake version
project(chip-8 C)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

set(CMAKE_C_STANDARD 23)
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

if(EMSCRIPTEN)
# remove --closure-args="--compilation_level SIMPLE" when https://github.com/emscripten-core/emscripten/issues/16706 is fixed
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -sGL_ENABLE_GET_PROC_ADDRESS -O2 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -s ASYNCIFY -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=1 -s MINIFY_HTML=0 -s ENVIRONMENT=web,worker -sFILESYSTEM=0"
  )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/shell.html"
  )
  set(CMAKE_EXECUTABLE_SUFFIX ".html")

  # Remove the -rdynamic flag because otherwise emscripten does not generate
  # HTML+JS+WASM files, only a non-working and fat HTML
  string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS
                 "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
endif()


cpmaddpackage("gh:libsdl-org/SDL#a509771a87559ca040a40b5fef932a8220343f7a")

if(FILE_OPEN AND NOT EMSCRIPTEN)
  add_compile_definitions(FILE_OPEN=1)
  cpmaddpackage("gh:btzy/nativefiledialog-extended#master")
endif()

add_executable(chip-8 ${SOURCES})

if(EMSCRIPTEN)
set_target_properties(chip-8 PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/shell.html)
endif()

if(BUILD_SHARED_LIBS AND NOT EMSCRIPTEN)
  target_link_libraries(chip-8 SDL3-shared Threads::Threads
                        $<$<BOOL:${FILE_OPEN}>:nfd>)
else()
  target_link_libraries(chip-8 SDL3-static Threads::Threads $<$<BOOL:${FILE_OPEN}>:nfd>)
endif()

add_compile_options("Wall" "Wpedantic")

target_include_directories(chip-8 PUBLIC include)

option(SUPPORT_CLANGD "Support clangd" ON)
if(SUPPORT_CLANGD)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CLANGD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/cmake/.clangd.in)
  set(CLANGD_DEST ${CMAKE_CURRENT_SOURCE_DIR}/.clangd)
  configure_file(${CLANGD_SRC} ${CLANGD_DEST} @ONLY)
endif()
