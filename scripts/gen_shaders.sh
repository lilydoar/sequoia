#!/bin/zsh

set -e
setopt nullglob # Prevent error out if no files match a glob

SHADER_SRC_DIR="res/shaders"
SHADER_OUT_DIR="res/gen/shaders"

rm -r "$SHADER_OUT_DIR"
mkdir -p "$SHADER_OUT_DIR"

translate_shaders() {
    local stage=$1
    
    for file in "$SHADER_SRC_DIR"/*.$stage.hlsl; do
        [[ -f "$file" ]] || continue
        
        echo "Translating $file" >&2
        
        filename=$(basename "$file" .hlsl)
        
        shadercross "$file" -o "$SHADER_OUT_DIR/${filename}.$stage.spv"
        shadercross "$file" -o "$SHADER_OUT_DIR/${filename}.$stage.msl"
        shadercross "$file" -o "$SHADER_OUT_DIR/${filename}.$stage.dxil"
        shadercross "$file" -o "$SHADER_OUT_DIR/${filename}.$stage.json"
    done
}

translate_shaders "vert"
translate_shaders "frag"
translate_shaders "comp"
