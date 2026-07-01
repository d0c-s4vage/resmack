include(${CMAKE_CURRENT_SOURCE_DIR}/variables.cmake)

project(resmack-cli
  VERSION 0.0.1
  DESCRIPTION "CLI interface for resmack (this generates the fuzzing executable)"
)

file(GLOB_RECURSE RESMACK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

add_executable(${RESMACK_CLI} ${RESMACK_SOURCES})
target_link_libraries(${RESMACK_CLI}
  PUBLIC ${LIBRESMACK_FUZZ_SHARED}
)

install(
    TARGETS ${RESMACK_CLI}
    EXPORT ${RESMACK_CLI}_targets
    FILE_SET HEADERS DESTINATION include
    PUBLIC_HEADER
        DESTINATION include
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
