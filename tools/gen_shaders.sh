#!/bin/bash

set -e

rm -r assets/gen/shaders

# mkdir -p assets/gen/shaders/dxil
mkdir -p assets/gen/shaders/metal
# mkdir -p assets/gen/shaders/spirv

# for file in assets/shaders/*.slang; do
#     slangc "$file" -target dxil -o "assets/gen/shaders/dxil/$(basename "$file" .slang).dxil"
# done

for file in assets/shaders/*.slang; do
    slangc "$file" -target metal -o "assets/gen/shaders/metal/$(basename "$file" .slang).metal"
done

# for file in assets/shaders/*.slang; do
#     slangc "$file" -target spirv -o "assets/gen/shaders/spirv/$(basename "$file" .slang).spirv"
# done
