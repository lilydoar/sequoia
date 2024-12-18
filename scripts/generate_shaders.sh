#!/bin/bash

set -e

# Clean the output folders
rm -r shaders/metal
mkdir -p shaders/metal

# Compile slang shaders to metal
for file in shaders/slang/*.slang; do
    slangc "$file" -target metal -o "shaders/metal/$(basename "$file" .slang).metal"
done
