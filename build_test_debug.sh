#!/usr/bin/env bash


CMD=(
    clang++
        -Ofast
        -march=native
        -fsanitize-coverage=trace-pc-guard
        -std=c++20
        -I ws/libresmack/include
        test.cpp
        #alpha_grammar.cpp
        run_on_grammar.cpp
        build/debug/ws/libresmack_fuzz_main/libresmack_fuzz_main.a
        build/debug/ws/libresmack_fuzz/libresmack_fuzz.a
        build/debug/ws/libresmack/libresmack.a
)
"${CMD[@]}"
