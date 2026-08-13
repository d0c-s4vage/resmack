set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ASAN_FLAGS -fsanitize=address -fno-omit-frame-pointer)
set(COVERAGE_FLAGS
    -fsanitize-coverage=trace-pc-guard
    # -mllvm -sanitizer-coverage-gated-trace-callbacks=1
)

add_compile_options(
    "$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Werror>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Debug>>:-O0;-g;${ASAN_FLAGS}>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:-O3;-ffast-math;-march=native;-fno-sanitize=all>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:RelWithDebInfo>>:-O3;-ffast-math;-march=native;-fno-sanitize=all>"
)

# ASan needs to be on the link line too, not just compile
add_link_options(
    "$<$<CONFIG:Debug>:${ASAN_FLAGS}>"
)
