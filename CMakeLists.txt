cmake_minimum_required(VERSION 3.13)

project(resmack-main)

include(cmake/compiler_options.cmake)
include(cmake/debug.cmake)

set(CMAKE_INSTALL_PREFIX "/usr")

include(CTest)
enable_testing()

set(LIBRESMACK "libresmack")

# -------------------
# subprojects
# -------------------
add_subdirectory(ws/libresmack)
add_subdirectory(ws/libresmack_fuzz)
add_subdirectory(ws/resmack)
add_subdirectory(ws/libresmack_fuzz_main)
add_subdirectory(ws/resmack_perf)
