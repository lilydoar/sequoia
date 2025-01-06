// Render contains description types for resource creation

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Pipeline = @import("../sdl_v2/pipeline.zig");

const Self = @This();

desc: Pipeline.Desc,
pipeline: *sdl.SDL_GPUGraphicsPipeline,

const DrawCmd = union(enum) {
    primitives: struct {
        num_vertices: u32,
        num_instances: u32,
        first_vertex: u32,
        first_instance: u32,
    },
    primitives_indexed: struct {
        // FIXME: This belongs with the index buffer, not the draw call
        index_size: sdl.SDL_GPUIndexElementSize,
        num_indices: u32,
        num_instances: u32,
        first_index: u32,
        vertex_offset: u32,
        first_instance: u32,
    },
};

// pub fn init()

// TODO:
// pub fn bind_and_draw()
