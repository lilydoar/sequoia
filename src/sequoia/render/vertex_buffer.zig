const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const TransferBuffer = @import("transfer_buffer.zig");

pub const Descriptor = struct {
    capacity: u32,
    attributes: std.ArrayList(sdl.SDL_GPUVertexElementFormat),

    pub fn pitch(self: Descriptor) u32 {
        var size: u32 = 0;
        for (self.attributes.items) |attr| {
            size += vertexElementSize(attr);
        }
        return size;
    }
};

const Self = @This();

desc: Descriptor,
ptr: *sdl.SDL_GPUBuffer,
size: u32,

pub fn init(
    alloc: std.mem.Allocator,
    device: *sdl.SDL_GPUDevice,
    capacity: u32,
    attribs: []const sdl.SDL_GPUVertexElementFormat,
) !Self {
    var attributes = std.ArrayList(sdl.SDL_GPUVertexElementFormat).init(alloc);
    try attributes.appendSlice(attribs);

    return Self{
        .desc = .{
            .capacity = capacity,
            .attributes = attributes,
        },
        .ptr = sdl.SDL_CreateGPUBuffer(device, &.{
            .usage = sdl.SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = capacity,
        }) orelse return error.CreateGPUBuffer,
        .size = 0,
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUBuffer(device, self.ptr);
    self.desc.attributes.deinit();
}

pub fn bind(self: Self, pass: *sdl.SDL_GPURenderPass, slot: u32) void {
    sdl.SDL_BindGPUVertexBuffers(pass, slot, &.{ .buffer = self.ptr }, 1);
}

pub fn upload(self: *Self, queue: *TransferBuffer, data: []u8) !void {
    if (@as(u32, @intCast(data.len)) > self.desc.capacity)
        return error.BufferTooSmall;
    try queue.stage(.{
        .data = data,
        .location = .{ .buf = self.ptr },
    });
    self.size = @intCast(data.len);
}

pub fn vertexElementSize(format: sdl.SDL_GPUVertexElementFormat) u32 {
    return switch (format) {
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_INT => @sizeOf(i32) * 1,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_INT2 => @sizeOf(i32) * 2,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_INT3 => @sizeOf(i32) * 3,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_INT4 => @sizeOf(i32) * 4,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_UINT => @sizeOf(u32) * 1,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_UINT2 => @sizeOf(u32) * 2,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_UINT3 => @sizeOf(u32) * 3,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_UINT4 => @sizeOf(u32) * 4,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT => @sizeOf(f32) * 1,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2 => @sizeOf(f32) * 2,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3 => @sizeOf(f32) * 3,
        sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4 => @sizeOf(f32) * 4,
        else => unreachable,
    };
}
