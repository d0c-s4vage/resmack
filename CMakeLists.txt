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
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# files
set(CMAKE_CXX_STANDARD 20)

# include
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/libs)

file(GLOB ITEMS
    ${PROJECT_SOURCE_DIR}/src/items/*.hpp
    ${PROJECT_SOURCE_DIR}/src/items/*.cpp)

file(GLOB CALCS
    ${PROJECT_SOURCE_DIR}/src/calc/*.hpp
    ${PROJECT_SOURCE_DIR}/src/calc/*.cpp)

set(MAIN_SOURCES ${PROJECT_SOURCE_DIR}/src/main.cpp 
            ${PROJECT_SOURCE_DIR}/src/rules.cpp
            ${PROJECT_SOURCE_DIR}/src/rules.hpp
            ${PROJECT_SOURCE_DIR}/libs/nlohmann/json.hpp
            ${ITEMS}
            ${CALCS})

# target
add_executable(resmack ${MAIN_SOURCES})

# https://medium.com/heuristics/c-application-development-part-3-cmakelists-txt-from-scratch-7678253e5e24
# add_subdirectory(./libs/json)
# target_link_libraries(resmack json)
