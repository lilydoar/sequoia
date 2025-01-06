const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Desc = struct {
    file: []const u8,
    entrypoint: []const u8,
    format: sdl.SDL_GPUShaderFormat,
    stage: sdl.SDL_GPUShaderStage,
    num_samplers: u32 = 0,
    num_storage_textures: u32 = 0,
    num_storage_buffers: u32 = 0,
    num_uniform_buffers: u32 = 0,
    vertex_buf_descriptions: ?[]sdl.SDL_GPUVertexBufferDescription,
    vertex_attributes: ?[]sdl.SDL_GPUVertexAttribute,
};

pub fn init(
    device: *sdl.SDL_GPUDevice,
    desc: Desc,
) !*sdl.SDL_GPUShader {
    var size: usize = undefined;
    const data = sdl.SDL_LoadFile(desc.file.ptr, &size) orelse
        return error.SDL_LoadFile;
    defer sdl.SDL_free(data);

    return sdl.SDL_CreateGPUShader(device, &.{
        .code_size = size,
        .code = @as([*]const u8, @ptrCast(data))[0..size].ptr,
        .entrypoint = desc.entrypoint.ptr,
        .format = desc.format,
        .stage = desc.stage,
        .num_samplers = desc.num_samplers,
        .num_storage_textures = desc.num_storage_textures,
        .num_storage_buffers = desc.num_storage_buffers,
        .num_uniform_buffers = desc.num_uniform_buffers,
    }) orelse return error.SDL_CreateGPUShader;
}

pub fn deinit(self: *sdl.SDL_GPUShader) void {
    sdl.SDL_ReleaseGPUShader(self);
}
