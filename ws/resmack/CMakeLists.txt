cmake_minimum_required(VERSION 3.0)

project(resmack)

option(BUILD_TEST "Build the unit tests" OFF)

set(RESMACK_CLI resmack)

include(${PROJECT_SOURCE_DIR}/../../compiler_options.cmake)

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

install(TARGETS ${RESMACK_CLI} RUNTIME DESTINATION /usr/bin)
