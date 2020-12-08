cmake_minimum_required(VERSION 3.0)

project(resmack-main)

set(CMAKE_INSTALL_PREFIX "/usr")

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(ws/libresmack)
add_subdirectory(ws/libresmack_fuzz)
add_subdirectory(ws/libresmack_fuzz_main)
add_subdirectory(ws/resmack_perf)
add_subdirectory(ws/resmack)
