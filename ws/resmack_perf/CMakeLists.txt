cmake_minimum_required(VERSION 3.0)

project(resmack_perf)

include(${PROJECT_SOURCE_DIR}/../../compiler_options.cmake)

if(NOT BUILD_TEST)
    include_directories(
        ${PROJECT_SOURCE_DIR}/../libresmack/include)

    add_executable(resmack_perf main.cpp)
    target_link_libraries(resmack_perf libresmack)
endif()
