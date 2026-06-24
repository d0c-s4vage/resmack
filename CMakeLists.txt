cmake_minimum_required(VERSION 3.0)

project(resmack-main)

include(CTest)
enable_testing()

set(CMAKE_INSTALL_PREFIX "/usr")

add_subdirectory(ws/libresmack)
add_subdirectory(ws/libresmack_fuzz)
add_subdirectory(ws/libresmack_fuzz_main)
add_subdirectory(ws/resmack_perf)
add_subdirectory(ws/resmack)
