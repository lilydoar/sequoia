const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL_gpu.h");
});

///
/// Upload data to the GPU
const Item = struct {
    // Data to be uploaded to the GPU
    data: *anyopaque,
    size: usize,
    // Location to upload data to
    location: union(enum) {
        buf: sdl.SDL_GPUBuffer,
        tex: struct {
            width: u32,
            height: u32,
            texture: sdl.SDL_GPUTexture,
        },
    },
};

items: std.ArrayList(Item),

const Self = @This();

pub fn init(alloc: std.mem.Allocator) Self {
    return .{
        .items = std.ArrayList(Item).init(alloc),
    };
}
pub fn deinit(self: Self) void {
    self.items.deinit();
}

/// The number of uploads staged
pub fn staged(self: Self) usize {
    return self.items.items.len;
}

/// Stage data for upload to the GPU
pub fn stage(self: *Self, item: Item) !void {
    try self.items.append(item);
}

pub fn flush(self: *Self, device: *sdl.SDL_GPUDevice) !void {
    const commandBuf = sdl.SDL_AcquireGPUCommandBuffer(device) orelse
        return error.SDL_AcquireGPUCommandBuffer;
    errdefer sdl.SDL_CancelGPUCommandBuffer(commandBuf);
    defer sdl.SDL_SubmitGPUCommandBuffer(commandBuf);

    const pass = sdl.SDL_BeginGPUCopyPass(commandBuf);
    defer sdl.SDL_EndGPUCopyPass(pass);

    while (self.items.items.len) {
        try self.upload_item(device, pass);
    }
}

pub fn pop_front(list: *std.ArrayList(Item)) ?Item {
    if (list.items.len == 0) return null;
    const item = list.orderedRemove(0);
    return item;
}
fn upload_item(
    self: *Self,
    device: *sdl.SDL_GPUDevice,
    pass: *sdl.SDL_GPUCopyPass,
) !void {
    const item = pop_front(self.items) orelse return;

    const buf = sdl.SDL_CreateGPUTransferBuffer(
        device,
        .{
            .usage = sdl.SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = item.size,
        },
    ) orelse return error.SDL_CreateGPUTransferBuffer;
    defer sdl.SDL_ReleaseGPUTransferBuffer(device, buf);

    const transfer = sdl.SDL_MapGPUTransferBuffer(
        device,
        buf,
        false,
    ) orelse return error.SDL_MapGPUTransferBuffer;
    defer sdl.SDL_UnmapGPUTransferBuffer(device, buf);

    @memcpy(transfer, item.data[0..item.size]);

    switch (item.location) {
        .Buffer => sdl.SDL_UploadToGPUBuffer(
            pass,
            .{ .transfer_buffer = buf },
            .{
                .buffer = item.location.buf,
                .size = item.size,
            },
            false,
        ),
        .Texture => sdl.SDL_UploadToGPUTexture(
            pass,
            .{
                .transfer_buffer = buf,
                .pixels_per_row = item.location.tex.width,
                .rows_per_layer = item.location.tex.height,
            },
            .{
                .texture = item.location.tex.texture,
                .w = item.location.tex.width,
                .h = item.location.tex.height,
                .d = 1, // default depth of a flat 2D texture
            },
            false,
        ),
    }
}
