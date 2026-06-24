cmake_minimum_required(VERSION 3.0)

project(libresmack_fuzz VERSION 0.0.1 DESCRIPTION "resmack fuzzing library")

option(BUILD_TEST "Build the unit tests" OFF)
option(DEBUG_IPC "Debug IPC waits/releases" OFF)

find_package(OpenSSL REQUIRED)

include(${CMAKE_SOURCE_DIR}/compiler_options.cmake)

set(RESMACK_FUZZ_LIB libresmack_fuzz)

find_package(libresmack)
include(GNUInstallDirs)

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

add_library(${RESMACK_FUZZ_LIB} SHARED ${LIBRESMACK_FUZZ_SOURCES})
set_target_properties(${RESMACK_FUZZ_LIB} PROPERTIES OUTPUT_NAME "resmack_fuzz")

add_library(${RESMACK_FUZZ_LIB}_static STATIC ${LIBRESMACK_FUZZ_SOURCES})
set_target_properties(${RESMACK_FUZZ_LIB}_static PROPERTIES OUTPUT_NAME "resmack_fuzz")

if (DEBUG_IPC)
  add_definitions(-DDEBUG_IPC)
endif()

target_link_libraries(
    ${RESMACK_FUZZ_LIB}
    ${RESMACK_LIB}
    pthread
    OpenSSL::Crypto
    unwind
    unwind-ptrace
    unwind-generic
)

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS ${RESMACK_FUZZ_LIB} EXPORT
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(test)
