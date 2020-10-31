cmake_minimum_required(VERSION 3.0)

project(resmack_perf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -flto")

find_package(libresmack)

include_directories(
    ${PROJECT_SOURCE_DIR}/../libresmack/include)

add_executable(resmack_perf main.cpp)
target_link_libraries(resmack_perf libresmack)
