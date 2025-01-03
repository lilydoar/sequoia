const std = @import("std");

const Context = @import("../sdl/context.zig");
const shader = @import("../sdl/shader.zig");
const Render = @import("render.zig");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn init(context: *Context) !Render {
    const vert = try context.shaders.register(
        "tri_vert",
        try shader.fromFile(
            context.device,
            "assets/gen/shaders/metal/triangle.metal",
            .{
                .entrypoint = "vertexMain",
                .format = sdl.SDL_GPU_SHADERFORMAT_MSL,
                .stage = sdl.SDL_GPU_SHADERSTAGE_VERTEX,
                .num_storage_buffers = 1,
            },
        ),
    );
    const frag = try context.shaders.register(
        "tri_frag",
        try shader.fromFile(
            context.device,
            "assets/gen/shaders/metal/triangle.metal",
            .{
                .entrypoint = "fragmentMain",
                .format = sdl.SDL_GPU_SHADERFORMAT_MSL,
                .stage = sdl.SDL_GPU_SHADERSTAGE_FRAGMENT,
            },
        ),
    );

    // Not certain where this information's init belongs.
    // I think it is strongly linked to the vertex shader.
    // But it may be slightly more generic than that
    const buffer_desc = [_]sdl.SDL_GPUVertexBufferDescription{
        .{
            .slot = 0,
            .pitch = @sizeOf(f32) * 6,
            .input_rate = sdl.SDL_GPU_VERTEXINPUTRATE_VERTEX,
        },
    };
    const vert_attribs = [_]sdl.SDL_GPUVertexAttribute{
        .{
            .location = 0,
            .buffer_slot = 0,
            .format = sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = 0,
        },
        .{
            .location = 1,
            .buffer_slot = 0,
            .format = sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = @sizeOf(f32) * 2,
        },
    };

    const pipeline = try context.pipelines.register(
        "triangle",
        sdl.SDL_CreateGPUGraphicsPipeline(
            context.device,
            &.{
                .vertex_shader = context.shaders.get(vert),
                .fragment_shader = context.shaders.get(frag),
                .primitive_type = sdl.SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                .vertex_input_state = .{
                    .vertex_buffer_descriptions = &buffer_desc,
                    .num_vertex_buffers = buffer_desc.len,
                    .vertex_attributes = &vert_attribs,
                    .num_vertex_attributes = vert_attribs.len,
                },
                .rasterizer_state = .{},
                .multisample_state = .{},
                .depth_stencil_state = .{},
                .target_info = .{},
            },
        ) orelse return error.SDL_CreateGPUGraphicsPipeline,
    );

    return .{
        .vert = vert,
        .frag = frag,
        .pipeline = pipeline,
    };
}
