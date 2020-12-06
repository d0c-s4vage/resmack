cmake_minimum_required(VERSION 3.0)

project(resmack)

option(BUILD_TEST "Build the unit tests" OFF)

set(RESMACK_CLI resmack)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -flto")

#find_package(libresmack)
#find_package(libresmack_fuzz)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/../libresmack/include
    ${PROJECT_SOURCE_DIR}/../libresmack_fuzz/include)

file(GLOB RESMACK_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp)

add_executable(${RESMACK_CLI} ${RESMACK_SOURCES})
target_link_libraries(${RESMACK_CLI} libresmack libresmack_fuzz)

#if(BUILD_TEST)
#  add_subdirectory(test)
#endif()
