#!/usr/bin/env bash


CMD=(
    build/release/ws/resmack/resmack cc
      $@
      --
        -std=c++20
        -I ws/libresmack/include
        test.cpp
        #alpha_grammar.cpp
        run_on_grammar.cpp
)
"${CMD[@]}"
