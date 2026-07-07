set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
set(COVERAGE_FLAGS
        "-fsanitize-coverage=trace-pc-guard"
        "-mllvm" "-sanitizer-coverage-gated-trace-callbacks=1"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g ${ASAN_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -march=native")

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
