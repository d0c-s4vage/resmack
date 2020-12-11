#!/usr/bin/env bash


CMD=(
    build/release/resmack cc
      $@
      --
        -std=c++20
        test.cpp
        run_on_grammar.cpp
)
"${CMD[@]}"
