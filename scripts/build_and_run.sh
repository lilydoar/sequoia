#!/bin/bash

DEBUG=0
while getopts "d" flag; do
    case "${flag}" in
        d) DEBUG=1;;
    esac
done

watchexec -e zig zig build &
PID=$!

if [ $DEBUG -eq 1 ]; then
    lldb ./zig-out/bin/game
else 
    ./zig-out/bin/game
fi

kill $PID
