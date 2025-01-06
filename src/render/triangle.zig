const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Shader = @import("../sdl_v2/shader.zig");
const Pipeline = @import("../sdl_v2/pipeline.zig");
const Binding = @import("../sdl_v2/binding.zig");
const Render = @import("../sdl_v2/render.zig");
const Buffer = @import("../sdl_v2/buffer.zig");

pub fn init(
    device: *sdl.SDL_GPUDevice,
    swapchain_format: sdl.SDL_GPUTextureFormat,
) !Render {
    const vert = Shader.Desc{
        .file = "assets/gen/shaders/metal/triangle.metal",
        .entrypoint = "vertexMain",
        .format = sdl.SDL_GPU_SHADERFORMAT_MSL,
        .stage = sdl.SDL_GPU_SHADERSTAGE_VERTEX,
        .num_storage_buffers = 1,
        .vertex_buf_descriptions = &[_]sdl.SDL_GPUVertexBufferDescription{
            .{
                .slot = 0,
                .pitch = @sizeOf(f32) * 6,
                .input_rate = sdl.SDL_GPU_VERTEXINPUTRATE_VERTEX,
            },
        },
        .vertex_attributes = &[_]sdl.SDL_GPUVertexAttribute{
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
        },
    };

    const frag = Shader.Desc{
        .file = "assets/gen/shaders/metal/triangle.metal",
        .entrypoint = "fragmentMain",
        .format = sdl.SDL_GPU_SHADERFORMAT_MSL,
        .stage = sdl.SDL_GPU_SHADERSTAGE_FRAGMENT,
    };

    const vert_buf = try Buffer.init(device, .{
        .type = sdl.SDL_GPU_BUFFER_TYPE_VERTEX,
        .size = @sizeOf(f32) * 6 * 3,
    });

    const pipeline = Pipeline.Desc{
        .vertex_shader = vert,
        .fragment_shader = frag,
        .bindings = []Binding.Desc{
            .{
                .resource = .{ .buffer = vert_buf },
            },
        },
        .primitive_type = sdl.SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info = .{
            .color_target_descriptions = .{
                .format = swapchain_format,
                .blend_state = .{
                    .enable_blend = true,
                    .color_blend_op = sdl.SDL_GPU_BLENDOP_ADD,
                    .alpha_blend_op = sdl.SDL_GPU_BLENDOP_ADD,
                    .src_color_blendfactor = sdl.SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .src_alpha_blendfactor = sdl.SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = sdl.SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .dst_alpha_blendfactor = sdl.SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                },
            },
            .num_color_targets = 1,
        },
    };

    return .{
        .desc = pipeline,
        .pipeline = try Pipeline.init(device, pipeline),
    };
}
