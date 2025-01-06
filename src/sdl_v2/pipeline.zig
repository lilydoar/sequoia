const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Shader = @import("shader.zig");
const Binding = @import("binding.zig");

pub const Desc = struct {
    vertex_shader: Shader.Desc,
    fragment_shader: Shader.Desc,
    bindings: ?[]Binding.Desc = null,
    primitive_type: sdl.SDL_GPUPrimitiveType,
    rasterizer_state: sdl.SDL_GPURasterizerState,
    multisample_state: sdl.SDL_GPUMultisampleState,
    depth_stencil_state: sdl.SDL_GPUDepthStencilState,
    target_info: sdl.SDL_GPUGraphicsPipelineTargetInfo,
};

pub fn init(
    device: *sdl.SDL_GPUDevice,
    desc: Desc,
) !*sdl.SDL_GPUGraphicsPipeline {
    const vert = try Shader.init(desc.vertex_shader);
    defer Shader.deinit(vert);
    const frag = try Shader.init(desc.fragment_shader);
    defer Shader.deinit(frag);

    const descriptions = desc.vertex_shader.vertex_buf_descriptions.?;
    const attributes = desc.vertex_shader.vertex_attributes.?;

    return sdl.SDL_CreateGPUGraphicsPipeline(
        device,
        &.{
            .vertex_shader = vert,
            .fragment_shader = frag,
            .primitive_type = desc.primitive_type,
            .vertex_input_state = .{
                .vertex_buffer_descriptions = descriptions,
                .num_vertex_buffers = descriptions.len,
                .vertex_attributes = attributes,
                .num_vertex_attributes = attributes.len,
            },
            .rasterizer_state = desc.rasterizer_state,
            .multisample_state = desc.multisample_state,
            .depth_stencil_state = desc.depth_stencil_state,
            .target_info = desc.target_info,
        },
    ) orelse return error.SDL_CreateGPUGraphicsPipeline;
}

pub fn deinit(self: *sdl.SDL_GPUGraphicsPipeline) void {
    sdl.SDL_ReleaseGPUGraphicsPipeline(self);
}
