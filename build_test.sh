#!/usr/bin/env bash


CMD=(
    build/release/ws/resmack/resmack cc --
        -g
        -std=c++20
        -Ofast
        -march=native
        -I ws/libresmack/include
        test.cpp
        #alpha_grammar.cpp
        run_on_grammar.cpp
)
"${CMD[@]}"
