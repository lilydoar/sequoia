const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Info = struct {
    entrypoint: []const u8,
    format: sdl.SDL_GPUShaderFormat,
    stage: sdl.SDL_GPUShaderStage,
    num_samplers: u32 = 0,
    num_storage_textures: u32 = 0,
    num_storage_buffers: u32 = 0,
    num_uniform_buffers: u32 = 0,
};

pub fn fromFile(device: *sdl.SDL_GPUDevice, file: []const u8, info: Info) !*sdl.SDL_GPUShader {
    var size: usize = undefined;
    const data = sdl.SDL_LoadFile(file.ptr, &size) orelse return error.SDL_LoadFile;
    defer sdl.SDL_free(data);

    return sdl.SDL_CreateGPUShader(device, &.{
        .code_size = size,
        .code = @as([*]const u8, @ptrCast(data))[0..size].ptr,
        .entrypoint = info.entrypoint.ptr,
        .format = info.format,
        .stage = info.stage,
        .num_samplers = info.num_samplers,
        .num_storage_textures = info.num_storage_textures,
        .num_storage_buffers = info.num_storage_buffers,
        .num_uniform_buffers = info.num_uniform_buffers,
    }) orelse return error.SDL_CreateGPUShader;
}
