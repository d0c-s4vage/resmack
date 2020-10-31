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

add_subdirectory(ws/libresmack)
add_subdirectory(ws/resmack_perf)
