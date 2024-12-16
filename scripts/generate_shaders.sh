#!/bin/bash

set -e

# Make sure the output folder exists
mkdir -p shaders/metal
mkdir -p shaders/glsl
mkdir -p shaders/hlsl

# Compile slang shaders to glsl
for file in shaders/slang/*.slang; do
    slangc "$file" -target glsl -o "shaders/glsl/$(basename "$file" .slang).glsl"
done

# Compile slang shaders to hlsl
for file in shaders/slang/*.slang; do
    slangc "$file" -target hlsl -o "shaders/hlsl/$(basename "$file" .slang).hlsl"
done

# Compile slang shaders to metal
for file in shaders/slang/*.slang; do
    slangc "$file" -target metal -o "shaders/metal/$(basename "$file" .slang).metal"
done
