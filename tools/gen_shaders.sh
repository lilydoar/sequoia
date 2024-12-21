#!/bin/bash

set -e

mkdir -p assets/gen/shaders/metal

# Compile slang shaders to metal
for file in assets/shaders/*.slang; do
    slangc "$file" -target metal -o "assets/gen/shaders/metal/$(basename "$file" .slang).metal"
done
