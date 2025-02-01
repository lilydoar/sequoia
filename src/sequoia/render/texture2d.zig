const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});
const zigimg = @import("zigimg");

const TransferBuffer = @import("transfer_buffer.zig");

const Self = @This();

ptr: *sdl.SDL_GPUTexture,
image: zigimg.Image,

pub fn fromFile(
    alloc: std.mem.Allocator,
    device: *sdl.SDL_GPUDevice,
    path: []const u8,
) !Self {
    const image = try zigimg.Image.fromFilePath(alloc, path);

    if (image.pixelFormat() != .rgba32) return error.UnsupportedImageFormat;

    const texture = sdl.SDL_CreateGPUTexture(device, &.{
        .type = sdl.SDL_GPU_TEXTURETYPE_2D,
        .format = sdl.SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = sdl.SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = @intCast(image.width),
        .height = @intCast(image.height),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    }) orelse return error.SDL_CreateGPUTexture;

    return Self{
        .ptr = texture,
        .image = image,
    };
}

pub fn deinit(self: *Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUTexture(device, self.ptr);
    self.image.deinit();
}

pub fn bind(
    self: Self,
    pass: *sdl.SDL_GPURenderPass,
    slot: u32,
    sampler: *sdl.SDL_GPUSampler,
) void {
    sdl.SDL_BindGPUFragmentSamplers(
        pass,
        slot,
        &.{ .texture = self.ptr, .sampler = sampler },
        1,
    );
}

pub fn upload(self: Self, transfer_buf: *TransferBuffer) !void {
    try transfer_buf.stage(.{
        .data = self.image.rawBytes(),
        .location = .{
            .tex2d = .{
                .ptr = self.ptr,
                .w = @intCast(self.image.width),
                .h = @intCast(self.image.height),
            },
        },
    });
}
