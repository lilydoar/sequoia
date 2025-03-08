#!/bin/bash

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <shader_src_dir>"
    exit 1
fi

SHADER_SRC_DIR="$1"
SHADER_GEN_DIR="$1/gen"

rm -rf "$SHADER_GEN_DIR"
mkdir -p "$SHADER_GEN_DIR"

for filename in $SHADER_SRC_DIR/*.hlsl; do
    if [ -f "$filename" ]; then
	basename=$(basename "$filename")
        shadercross "$filename" -o "$SHADER_GEN_DIR/${basename/.hlsl/.dxil}"
        shadercross "$filename" -o "$SHADER_GEN_DIR/${basename/.hlsl/.msl}"
        shadercross "$filename" -o "$SHADER_GEN_DIR/${basename/.hlsl/.spv}"
        shadercross "$filename" -o "$SHADER_GEN_DIR/${basename/.hlsl/.json}"
    fi
done

