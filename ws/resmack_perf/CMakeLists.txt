include(${libresmack_fuzz_SOURCE_DIR}/variables.cmake)

project(resmack_perf)

add_executable(resmack_perf main.cpp)
target_link_libraries(resmack_perf
  PUBLIC ${LIBRESMACK_FUZZ_SHARED}
)
