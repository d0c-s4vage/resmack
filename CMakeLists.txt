cmake_minimum_required(VERSION 3.0)

project(resmack-main)

# Generate compile_commands.json:
# cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1

## or cmake -DCMAKE_BUILD_TYPE=Release
#set(CMAKE_BUILD_TYPE Release)
## or cmake -DCMAKE_BUILD_TYPE=Debug
#set(CMAKE_BUILD_TYPE Debug)
## or cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
#set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_INSTALL_PREFIX "/usr")

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


add_subdirectory(ws/libresmack)
add_subdirectory(ws/libresmack_fuzz)
add_subdirectory(ws/libresmack_fuzz_main)
add_subdirectory(ws/resmack_perf)
add_subdirectory(ws/resmack)
