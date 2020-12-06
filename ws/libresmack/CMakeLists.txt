cmake_minimum_required(VERSION 3.0)

project(libresmack VERSION 0.0.1 DESCRIPTION "resmack core grammar library")

option(BUILD_TEST "Build the unit tests" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -flto")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native")

set(RESMACK_LIB libresmack)

include(GNUInstallDirs)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/fmt/include)

file(GLOB LIBRESMACK_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp
    ${PROJECT_SOURCE_DIR}/src/calc/*.cpp
    ${PROJECT_SOURCE_DIR}/src/calc/*.hpp
    ${PROJECT_SOURCE_DIR}/src/items/*.cpp
    ${PROJECT_SOURCE_DIR}/src/items/*.hpp)

add_library(${RESMACK_LIB} SHARED ${LIBRESMACK_SOURCES})
set_target_properties(${RESMACK_LIB} PROPERTIES OUTPUT_NAME "resmack")

add_subdirectory(${PROJECT_SOURCE_DIR}/libs/fmt)
target_link_libraries(${RESMACK_LIB} fmt)

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS ${RESMACK_LIB} EXPORT
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_TEST)
  add_subdirectory(test)
    #add_executable(${RESMACK_PERF_EXE} ${PROJECT_SOURCE_DIR}/src/mains/perf.cpp)
    #target_link_libraries(${RESMACK_PERF_EXE} ${RESMACK_LIB})

    #add_executable(${RESMACK_EXE} ${PROJECT_SOURCE_DIR}/src/mains/resmack.cpp)
    #target_link_libraries(${RESMACK_EXE} ${RESMACK_LIB})
endif()
