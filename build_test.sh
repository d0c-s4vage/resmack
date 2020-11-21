#!/usr/bin/env bash


CMD=(
    build/release/ws/resmack/resmack cc --
        #-Ofast
        #-march=native
        -I ws/libresmack/include
        test.cpp
)
"${CMD[@]}"
