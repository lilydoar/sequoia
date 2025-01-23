#!/bin/zsh

set -e
setopt nullglob # Prevent error out if no files match a glob

SHADER_SRC_DIR="res/shaders"
SHADER_OUT_DIR="gen/shaders"

rm -rf "$SHADER_OUT_DIR"
mkdir -p "$SHADER_OUT_DIR"

translate_shaders() {
    local stage=$1
    
    for file in "$SHADER_SRC_DIR"/*.$stage.hlsl; do
        [[ -f "$file" ]] || continue
        
        echo "Translating $file" >&2
        
        filename=$(basename "$file" .hlsl)
        
        shadercross "$file" -o "$SHADER_OUT_DIR/${filename}.dxil"
        shadercross "$file" -o "$SHADER_OUT_DIR/${filename}.json"
        shadercross "$file" -o "$SHADER_OUT_DIR/${filename}.msl"
        shadercross "$file" -o "$SHADER_OUT_DIR/${filename}.spv"
    done
}

translate_shaders "vert"
translate_shaders "frag"
translate_shaders "comp"
