#!/bin/bash

watchexec -e zig zig build &
PID=$!

./zig-out/bin/sequoia
kill $PID

