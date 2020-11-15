cmake_minimum_required(VERSION 3.0)

project(libresmack_fuzz_main)

option(BUILD_TEST "Build the unit tests" OFF)

set(RESMACK_FUZZ_MAIN_LIB libresmack_fuzz_main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -flto")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native")

find_package(libresmack)
find_package(libresmack_fuzz)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/../libresmack/include
    ${PROJECT_SOURCE_DIR}/../libresmack_fuzz/include)

file(GLOB LIBRESMACK_FUZZ_MAIN_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp)

add_library(${RESMACK_FUZZ_MAIN_LIB} STATIC ${LIBRESMACK_FUZZ_MAIN_SOURCES})
target_link_libraries(${RESMACK_FUZZ_MAIN_LIB} libresmack libresmack_fuzz)
set_target_properties(${RESMACK_FUZZ_MAIN_LIB} PROPERTIES OUTPUT_NAME "resmack_fuzz_main")

#if(BUILD_TEST)
#  add_subdirectory(test)
#endif()
