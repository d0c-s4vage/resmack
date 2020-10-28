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

# options
option(BUILD_TEST "Build the unit tests" OFF)

# flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -flto")

# files
set(CMAKE_CXX_STANDARD 20)

# vars
set(RESMACK_LIB resmack)
set(RESMACK_EXE resmack_main)

# include
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/fmt/include)

file(GLOB ITEMS
    ${PROJECT_SOURCE_DIR}/src/items/*.hpp
    ${PROJECT_SOURCE_DIR}/src/items/*.cpp)

file(GLOB CALCS
    ${PROJECT_SOURCE_DIR}/src/calc/*.hpp
    ${PROJECT_SOURCE_DIR}/src/calc/*.cpp)

file(GLOB ROOT_FILES
    ${PROJECT_SOURCE_DIR}/src/*.hpp
    ${PROJECT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM ROOT_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp)

set(MAIN_SOURCES
    ${ITEMS}
    ${CALCS}
    ${ROOT_FILES}
    ${PROJECT_SOURCE_DIR}/libs/nlohmann/json.hpp)

# target
add_library(${RESMACK_LIB} STATIC ${MAIN_SOURCES})

# https://medium.com/heuristics/c-application-development-part-3-cmakelists-txt-from-scratch-7678253e5e24
# add_subdirectory(${PROJECT_SOURCE_DIR}/libs/json)
# target_link_libraries(resmack json)

add_subdirectory(${PROJECT_SOURCE_DIR}/libs/fmt)
target_link_libraries(${RESMACK_LIB} fmt)


if(BUILD_TEST)
  add_subdirectory(test)
else()
  target_link_libraries(${RESMACK_EXE} ${RESMACK_LIB})
  add_executable(${RESMACK_EXE} ${PROJECT_SOURCE_DIR}/src/main.cpp)
endif()
