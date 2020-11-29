cmake_minimum_required(VERSION 3.0)

project(libresmack_fuzz)

option(BUILD_TEST "Build the unit tests" OFF)

set(RESMACK_FUZZ_LIB libresmack_fuzz)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -flto")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native")

#find_package(libresmack)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../libresmack/include)

file(GLOB LIBRESMACK_FUZZ_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp
    ${PROJECT_SOURCE_DIR}/src/targets/*.cpp
    ${PROJECT_SOURCE_DIR}/src/targets/*.hpp
    ${PROJECT_SOURCE_DIR}/src/feedbacks/*.cpp
    ${PROJECT_SOURCE_DIR}/src/feedbacks/*.hpp
    ${PROJECT_SOURCE_DIR}/src/states/*.cpp
    ${PROJECT_SOURCE_DIR}/src/states/*.hpp
    ${PROJECT_SOURCE_DIR}/src/corpora/*.cpp
    ${PROJECT_SOURCE_DIR}/src/corpora/*.hpp
    ${PROJECT_SOURCE_DIR}/src/trace_targets/*.cpp
    ${PROJECT_SOURCE_DIR}/src/trace_targets/*.hpp)

add_library(${RESMACK_FUZZ_LIB} STATIC ${LIBRESMACK_FUZZ_SOURCES})

#add_library(unwind STATIC IMPORTED GLOBAL)
#find_library(unwind-generic libunwind-generic.a STATIC)
#find_library(unwind-ptrace libunwind-ptrace.a STATIC)

target_link_libraries(${RESMACK_FUZZ_LIB} libresmack)
set_target_properties(${RESMACK_FUZZ_LIB} PROPERTIES OUTPUT_NAME "resmack_fuzz")

if(BUILD_TEST)
  add_subdirectory(test)
endif()
