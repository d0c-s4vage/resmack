#!/usr/bin/env bash

CMD=(
    clang++
    -std=c++20
    -fsanitize-coverage=trace-pc-guard
    -Ofast
    -march=native
        build/release/ws/libresmack_fuzz_main/libresmack_fuzz_main.a
        build/release/ws/libresmack/libresmack.a
        build/release/ws/libresmack_fuzz/libresmack_fuzz.a
        test.cpp
)
"${CMD[@]}"
