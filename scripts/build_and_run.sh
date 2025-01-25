#!/bin/bash

set -e

DEBUG=0
while getopts "d" flag; do
    case "${flag}" in
        d) DEBUG=1;;
    esac
done

zig build

if [ $DEBUG -eq 1 ]; then
    lldb ./zig-out/bin/game
else 
    watchexec -e zig zig build &
    PID=$!

    ./zig-out/bin/game
    kill $PID
fi

