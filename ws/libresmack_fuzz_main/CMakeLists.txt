include("${CMAKE_CURRENT_SOURCE_DIR}/variables.cmake")

project("${LIBRESMACK_FUZZ_MAIN}"
  VERSION 0.0.1
  DESCRIPTION "resmack fuzzing library with 'main'"
)

file(GLOB_RECURSE LIBRESMACK_FUZZ_MAIN_PRIVATE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# -------------------------------------------
# OBJECT FILE (so we only have to build once)
# -------------------------------------------

add_library(${LIBRESMACK_FUZZ_MAIN_OBJ} STATIC)
target_sources(
    ${LIBRESMACK_FUZZ_MAIN_OBJ}
    PRIVATE
        ${LIBRESMACK_FUZZ_MAIN_PRIVATE_SRC}
)
target_link_libraries(
    ${LIBRESMACK_FUZZ_MAIN_OBJ}
    PUBLIC ${LIBRESMACK_SHARED}
    PUBLIC ${LIBRESMACK_FUZZ_SHARED}
)

# -------------------------------------------
# static and shared libs that use the object
# file
# -------------------------------------------

add_library("${LIBRESMACK_FUZZ_MAIN_STATIC}" STATIC $<TARGET_OBJECTS:${LIBRESMACK_FUZZ_MAIN_OBJ}>)
set_target_properties(${LIBRESMACK_FUZZ_MAIN_STATIC} PROPERTIES
    OUTPUT_NAME "resmack_fuzz_main-static"
)
target_link_libraries(
    "${LIBRESMACK_FUZZ_MAIN_STATIC}"
    PUBLIC ${LIBRESMACK_FUZZ_MAIN_OBJ}
)

# -------------------------------------------

install(
    TARGETS ${LIBRESMACK_FUZZ_MAIN_STATIC}
    EXPORT ${LIBRESMACK_FUZZ_MAIN}_targets
    FILE_SET HEADERS DESTINATION include
    PUBLIC_HEADER
        DESTINATION include
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
