// A Render struct contains all information necessary to make a render call

const manager = @import("../sdl/resource_management.zig");
const shader = @import("../sdl/shader.zig");

const shaderID = manager.ResourceLib(*sdl.SDL_GPUShader).ResourceID;
const pipelineID = manager.ResourceLib(*sdl.SDL_GPUGraphicsPipeline).ResourceID;

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

vert: shaderID,
frag: shaderID,
pipeline: pipelineID,
