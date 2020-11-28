#!/usr/bin/env bash


CMD=(
    clang++
        -Ofast
        -march=native
        -fsanitize-coverage=trace-pc-guard
        -fsanitize=address
        -std=c++20
        -I ws/libresmack/include
        test.cpp
        #alpha_grammar.cpp
        run_on_grammar.cpp
        build/release-syms/ws/libresmack_fuzz_main/libresmack_fuzz_main.a
        build/release-syms/ws/libresmack_fuzz/libresmack_fuzz.a
        build/release-syms/ws/libresmack/libresmack.a
        -o ./a.out
)
"${CMD[@]}"
