include(CTest)

cmake_minimum_required(VERSION 3.0)

project(libresmack VERSION 0.0.1 DESCRIPTION "resmack core grammar library")

option(BUILD_TEST "Build the unit tests" OFF)

include(${PROJECT_SOURCE_DIR}/../../compiler_options.cmake)

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

add_library(${RESMACK_LIB}_static STATIC ${LIBRESMACK_SOURCES})
set_target_properties(${RESMACK_LIB}_static PROPERTIES OUTPUT_NAME "resmack")

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

add_subdirectory(test)
