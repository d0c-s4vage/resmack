#!/usr/bin/env bash

make release

export LD_LIBRARY_PATH=build/release

CMD=(
    build/release/resmack cc
      # --asan
      $@
      --
        -std=c++20
        test.cpp
        run_on_grammar.cpp
        -Lbuild/release
)
"${CMD[@]}"
