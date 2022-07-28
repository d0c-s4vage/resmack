#!/usr/bin/env bash

make debug

export LD_LIBRARY_PATH=build/debug

CMD=(
clang++
    -fno-omit-frame-pointer
    -Iws/libresmack/include
    -lpthread
    -lcrypto
    -lunwind
    -lunwind-ptrace
    -lunwind-generic
    -lresmack_fuzz_main
    -lresmack_fuzz
    -lresmack
    -ggdb
    # -Ofast
    -march=native
    -fsanitize-coverage=trace-pc-guard
    -std=c++20
    test.cpp
    run_on_grammar.cpp
    -Lbuild/debug
    -fsanitize=address
)
"${CMD[@]}"
