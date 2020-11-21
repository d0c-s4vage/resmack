#!/usr/bin/env bash


CMD=(
    build/release/ws/resmack/resmack cc --
        -Ofast
        -march=native
        test.cpp
)
"${CMD[@]}"
