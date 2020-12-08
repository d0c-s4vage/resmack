cmake_minimum_required(VERSION 3.0)

project(libresmack_fuzz_main VERSION 0.0.1 DESCRIPTION "resmack fuzzing library with 'main'")

option(BUILD_TEST "Build the unit tests" OFF)

set(RESMACK_FUZZ_MAIN_LIB libresmack_fuzz_main)

include(GNUInstallDirs)

include(${PROJECT_SOURCE_DIR}/../../compiler_options.cmake)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/../libresmack/include
    ${PROJECT_SOURCE_DIR}/../libresmack_fuzz/include)

file(GLOB LIBRESMACK_FUZZ_MAIN_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp)

add_library(${RESMACK_FUZZ_MAIN_LIB} STATIC ${LIBRESMACK_FUZZ_MAIN_SOURCES})
target_link_libraries(${RESMACK_FUZZ_MAIN_LIB} ${RESMACK_LIB} ${RESMACK_FUZZ_LIB})
set_target_properties(${RESMACK_FUZZ_MAIN_LIB} PROPERTIES OUTPUT_NAME "resmack_fuzz_main")

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS ${RESMACK_FUZZ_MAIN_LIB} EXPORT
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

# nothing to include for this one
#install(
#    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
#    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#if(BUILD_TEST)
#  add_subdirectory(test)
#endif()
