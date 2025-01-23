const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Descriptor = struct {
    size: u32,
    element_size: sdl.SDL_GPUIndexElementSize =
        sdl.SDL_GPU_INDEXELEMENTSIZE_16BIT,
};

const Self = @This();

desc: Descriptor,
ptr: *sdl.SDL_GPUBuffer,

pub fn init(
    device: *sdl.SDL_GPUDevice,
    desc: Descriptor,
) !Self {
    return Self{
        .desc = desc,
        .ptr = sdl.SDL_CreateGPUBuffer(device, &.{
            .usage = sdl.SDL_GPU_BUFFERUSAGE_INDEX,
            .size = desc.size,
        }) orelse return error.CreateGPUBuffer,
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUBuffer(device, self.ptr);
}

pub fn bind(
    self: *Self,
    pass: *sdl.SDL_GPURenderPass,
) void {
    sdl.SDL_BindGPUIndexBuffer(
        pass,
        &.{ .buffer = self.ptr },
        self.desc.element_size,
    );
}
