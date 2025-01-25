const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const TransferBuffer = @import("transfer_buffer.zig");

pub const Descriptor = struct {
    // FIXME: This doesn't need to be a struct. The size can be calculated from the format
    const Attribute = struct {
        size: u32,
        format: sdl.SDL_GPUVertexElementFormat,
    };

    capacity: u32,
    attribs: std.ArrayList(Attribute),

    pub fn pitch(self: Descriptor) u32 {
        var size: u32 = 0;
        for (self.attribs.items) |attr| {
            size += attr.size;
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
    attribs: []const Descriptor.Attribute,
) !Self {
    var attributes = std.ArrayList(Descriptor.Attribute).init(alloc);
    try attributes.appendSlice(attribs);

    return Self{
        .desc = .{
            .capacity = capacity,
            .attribs = attributes,
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
    self.desc.attribs.deinit();
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
