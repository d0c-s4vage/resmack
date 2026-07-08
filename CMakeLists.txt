cmake_minimum_required(VERSION 3.13)

project(resmack-main)

include(cmake/clang_tidy.cmake)
include(cmake/compiler_options.cmake)
include(cmake/debug.cmake)

set(CMAKE_INSTALL_PREFIX "/usr")

if(DEFINED ENV{DEBUG_MESSAGES})
  message(STATUS "Overriding DEBUG_MESSAGES: ${DEBUG_MESSAGES} -> $ENV{DEBUG_MESSAGES}")
  set(DEBUG_MESSAGES $ENV{DEBUG_MESSAGES})
endif()

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
