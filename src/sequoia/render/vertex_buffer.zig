const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Descriptor = struct {
    // FIXME: This doesn't need to be a struct. The size can be calculated from the format
    const Attribute = struct {
        size: u32,
        format: sdl.SDL_GPUVertexElementFormat,
    };

    size: u32,
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

pub fn init(
    alloc: std.mem.Allocator,
    device: *sdl.SDL_GPUDevice,
    size: u32,
    attribs: []const Descriptor.Attribute,
) !Self {
    var attributes = std.ArrayList(Descriptor.Attribute).init(alloc);
    try attributes.appendSlice(attribs);

    return Self{
        .desc = .{
            .size = size,
            .attribs = attributes,
        },
        .ptr = sdl.SDL_CreateGPUBuffer(device, &.{
            .usage = sdl.SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = size,
        }) orelse return error.CreateGPUBuffer,
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUBuffer(device, self.ptr);
    self.desc.attribs.deinit();
}

pub fn bind(self: *Self, pass: *sdl.SDL_GPURenderPass, slot: u32) void {
    sdl.SDL_BindGPUVertexBuffers(pass, slot, &.{ .buffer = self.ptr }, 1);
}
