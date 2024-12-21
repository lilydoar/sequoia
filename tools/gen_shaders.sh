#!/bin/bash

set -e

mkdir -p assets/gen/shaders/metal

# Compile slang shaders to metal
for file in shaders/slang/*.slang; do
    slangc "$file" -target metal -o "shaders/metal/$(basename "$file" .slang).metal"
done
