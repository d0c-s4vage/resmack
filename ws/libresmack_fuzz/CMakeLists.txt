include("${CMAKE_CURRENT_SOURCE_DIR}/variables.cmake")

project("${LIBRESMACK_FUZZ}"
    VERSION 0.0.1
    DESCRIPTION "resmack fuzzing library"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUNWIND REQUIRED libunwind)
pkg_check_modules(LIBUNWIND_PTRACE REQUIRED libunwind-ptrace)
pkg_check_modules(LIBUNWIND_GENERIC REQUIRED libunwind-generic)

find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE LIBRESMACK_FUZZ_PRIVATE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE LIBRESMACK_FUZZ_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/resmack/fuzz/*.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/include/resmack/fuzz/*.hpp"
)

# -------------------------------------------
# OBJECT FILE (so we only have to build once)
# -------------------------------------------

add_library(${LIBRESMACK_FUZZ_OBJ} OBJECT)
target_sources(
    ${LIBRESMACK_FUZZ_OBJ}
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_BINARY_DIR}/include"
        FILES
            ${LIBRESMACK_FUZZ_INCLUDES}
    PRIVATE
        ${LIBRESMACK_FUZZ_PRIVATE_SRC}
)

target_link_libraries(
    ${LIBRESMACK_FUZZ_OBJ}
    PUBLIC ${LIBRESMACK_SHARED}
    PUBLIC OpenSSL::Crypto
    PUBLIC ${LIBUNWIND_LIBRARIES}
    PUBLIC ${LIBUNWIND_PTRACE_LIBRARIES}
    PUBLIC ${LIBUNWIND_GENERIC_LIBRARIES}
)

# -------------------------------------------
# static and shared libs that use the object
# file
# -------------------------------------------

add_library("${LIBRESMACK_FUZZ_SHARED}" SHARED $<TARGET_OBJECTS:${LIBRESMACK_FUZZ_OBJ}>)
set_target_properties(${LIBRESMACK_FUZZ_SHARED} PROPERTIES
    OUTPUT_NAME "resmack_fuzz"
)
target_link_libraries(
    "${LIBRESMACK_FUZZ_SHARED}"
    PUBLIC ${LIBRESMACK_FUZZ_OBJ}
)

add_library("${LIBRESMACK_FUZZ_STATIC}" STATIC $<TARGET_OBJECTS:${LIBRESMACK_FUZZ_OBJ}>)
set_target_properties(${LIBRESMACK_FUZZ_STATIC} PROPERTIES
    OUTPUT_NAME "resmack_fuzz"
)
target_link_libraries(
    "${LIBRESMACK_FUZZ_STATIC}"
    PUBLIC ${LIBRESMACK_FUZZ_OBJ}
)

# -------------------------------------------

install(
    TARGETS ${LIBRESMACK_FUZZ_STATIC} ${LIBRESMACK_FUZZ_SHARED}
    EXPORT ${LIBRESMACK_FUZZ}_targets
    FILE_SET HEADERS DESTINATION include
    PUBLIC_HEADER
        DESTINATION include
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

if (BUILD_TESTING)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
