cmake_minimum_required(VERSION 3.0)

# Generate compile_commands.json:
# cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1

## or cmake -DCMAKE_BUILD_TYPE=Release
#set(CMAKE_BUILD_TYPE Release)
## or cmake -DCMAKE_BUILD_TYPE=Debug
#set(CMAKE_BUILD_TYPE Debug)
## or cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
#set(CMAKE_BUILD_TYPE RelWithDebInfo)

project(resmack)

# flags

# files
set(CMAKE_CXX_STANDARD 20)

# include
include_directories(./include ./src ./libs)

set(GLOB FIELDS src/fields/*.hpp)
set(MAIN_SOURCES src/main.cpp 
            src/rules.cpp
            src/rules.hpp
            libs/nlohmann/json.hpp
            ${FIELDS})

# target
add_executable(resmack ${MAIN_SOURCES})

# https://medium.com/heuristics/c-application-development-part-3-cmakelists-txt-from-scratch-7678253e5e24
# add_subdirectory(./libs/json)
# target_link_libraries(resmack json)

# 
