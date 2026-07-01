include("${CMAKE_CURRENT_SOURCE_DIR}/variables.cmake")

project("${LIBRESMACK}"
    VERSION 0.0.1
    DESCRIPTION "resmack core grammar library"
)

include(FetchContent)
FetchContent_Declare(
    fmt
    # Specify the commit you depend on and update it regularly.
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 12.2.0
)
FetchContent_MakeAvailable(fmt)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/resmack/debug.hpp.in"
    "include/resmack/debug.hpp"
)

file(GLOB_RECURSE LIBRESMACK_PRIVATE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

file(GLOB_RECURSE LIBRESMACK_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/resmack/*.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/include/resmack/*.hpp"
)

# -------------------------------------------
# OBJECT FILE (so we only have to build once)
# -------------------------------------------

add_library("${LIBRESMACK_OBJ}" OBJECT)
target_sources(
    ${LIBRESMACK_OBJ}
        PUBLIC
            FILE_SET HEADERS
                BASE_DIRS
                    "${CMAKE_CURRENT_SOURCE_DIR}/include"
                    "${CMAKE_CURRENT_BINARY_DIR}/include"
                FILES ${LIBRESMACK_INCLUDES}
        PRIVATE
            ${LIBRESMACK_PRIVATE_SRC}
)
target_link_libraries(
    "${LIBRESMACK_OBJ}"
    PUBLIC fmt::fmt-header-only
)

# -------------------------------------------
# static and shared libs that use the object
# file
# -------------------------------------------

add_library("${LIBRESMACK_SHARED}" SHARED $<TARGET_OBJECTS:${LIBRESMACK_OBJ}>)
set_target_properties(${LIBRESMACK_SHARED} PROPERTIES
    OUTPUT_NAME "resmack"
    #INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${LIBRESMACK_OBJ},INCLUDE_DIRECTORIES>
    #INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${LIBRESMACK_OBJ},INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(
    "${LIBRESMACK_SHARED}"
    PUBLIC ${LIBRESMACK_OBJ}
)

add_library("${LIBRESMACK_STATIC}" STATIC $<TARGET_OBJECTS:${LIBRESMACK_OBJ}>)
set_target_properties(${LIBRESMACK_STATIC} PROPERTIES
    OUTPUT_NAME "resmack"
    #INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${LIBRESMACK_OBJ},INCLUDE_DIRECTORIES>
    #INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${LIBRESMACK_OBJ},INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(
    "${LIBRESMACK_STATIC}"
    PUBLIC ${LIBRESMACK_OBJ}
)


# -------------------------------------------

install(
    TARGETS ${LIBRESMACK_STATIC} ${LIBRESMACK_SHARED}
    EXPORT ${LIBRESMACK}_targets
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
