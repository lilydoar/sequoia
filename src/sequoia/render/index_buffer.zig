const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const TransferQueue = @import("transfer_queue.zig");

pub const Descriptor = struct {
    capacity: u32,
    // FIXME: There is no validation that the data being passed in matches the
    // buffer's element size. Maybe element size is compile time parameter?
    element_size: sdl.SDL_GPUIndexElementSize =
        sdl.SDL_GPU_INDEXELEMENTSIZE_16BIT,
};

const Self = @This();

desc: Descriptor,
ptr: *sdl.SDL_GPUBuffer,
size: u32,

pub fn init(
    device: *sdl.SDL_GPUDevice,
    desc: Descriptor,
) !Self {
    return Self{
        .desc = desc,
        .ptr = sdl.SDL_CreateGPUBuffer(device, &.{
            .usage = sdl.SDL_GPU_BUFFERUSAGE_INDEX,
            .size = desc.capacity,
        }) orelse return error.CreateGPUBuffer,
        .size = 0,
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUBuffer(device, self.ptr);
}

pub fn bind(
    self: Self,
    pass: *sdl.SDL_GPURenderPass,
) void {
    sdl.SDL_BindGPUIndexBuffer(
        pass,
        &.{ .buffer = self.ptr },
        self.desc.element_size,
    );
}

pub fn upload(self: *Self, queue: *TransferQueue, data: []u8) !void {
    if (@as(u32, @intCast(data.len)) > self.desc.capacity)
        return error.BufferTooSmall;
    try queue.stage(.{
        .data = data,
        .location = .{ .buf = self.ptr },
    });
    self.size = @intCast(data.len);
}

pub fn count(self: Self) u32 {
    switch (self.desc.element_size) {
        sdl.SDL_GPU_INDEXELEMENTSIZE_16BIT => return self.size / @sizeOf(u16),
        sdl.SDL_GPU_INDEXELEMENTSIZE_32BIT => return self.size / @sizeOf(u32),
        else => unreachable,
    }
}
