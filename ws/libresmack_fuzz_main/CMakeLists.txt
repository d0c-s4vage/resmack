include(variables.cmake)

project(libresmack_fuzz_main
  VERSION 0.0.1
  DESCRIPTION "resmack fuzzing library with 'main'"
)

file(GLOB_RECURSE LIBRESMACK_FUZZ_MAIN_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

add_library(${LIBRESMACK_FUZZ_MAIN}
  STATIC ${LIBRESMACK_FUZZ_MAIN_SRC}
)
target_link_libraries(${LIBRESMACK_FUZZ_MAIN}
  PUBLIC ${LIBRESMACK_SHARED}
  PUBLIC ${LIBRESMACK_FUZZ_SHARED}
)
set_target_properties(${LIBRESMACK_FUZZ_MAIN} PROPERTIES
    OUTPUT_NAME "resmack_fuzz_main"
)

install(
    TARGETS ${LIBRESMACK_FUZZ_MAIN}
    EXPORT ${LIBRESMACK_FUZZ_MAIN}_targets
    FILE_SET HEADERS DESTINATION include
    PUBLIC_HEADER
        DESTINATION include
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
