const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Self = @This();

const Item = struct {
    data: []anyopaque,
    location: union(enum) {
        buf: *sdl.SDL_GPUBuffer,
        tex: struct {
            width: u32,
            height: u32,
            texture: *sdl.SDL_GPUTexture,
        },
    },
};

items: std.ArrayList(Item),

pub fn init(alloc: std.mem.Allocator) Self {
    return .{
        .items = std.ArrayList(Item).init(alloc),
    };
}

pub fn deinit(self: Self) void {
    self.items.deinit();
}

pub fn stage(self: *Self, item: Item) !void {
    try self.items.append(item);
}

pub fn staged(self: Self) usize {
    return self.items.items.len;
}

pub fn flush(self: *Self, device: *sdl.SDL_GPUDevice) !void {
    const commandBuf = sdl.SDL_AcquireGPUCommandBuffer(device) orelse
        return error.SDL_AcquireGPUCommandBuffer;
    errdefer _ = sdl.SDL_CancelGPUCommandBuffer(commandBuf);

    const pass = sdl.SDL_BeginGPUCopyPass(commandBuf) orelse
        return error.SDL_BeginGPUCopyPass;

    while (self.staged() > 0) {
        try self.upload_item(device, pass);
    }

    sdl.SDL_EndGPUCopyPass(pass);
    if (!sdl.SDL_SubmitGPUCommandBuffer(commandBuf)) {
        return error.SDL_SubmitGPUCommandBuffer;
    }
}

fn upload_item(
    self: *Self,
    device: *sdl.SDL_GPUDevice,
    pass: *sdl.SDL_GPUCopyPass,
) !void {
    const item = pop_front(&self.items) orelse return;

    const transfer_buf = sdl.SDL_CreateGPUTransferBuffer(
        device,
        &.{ .usage = sdl.SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD },
    ) orelse return error.SDL_CreateGPUTransferBuffer;
    defer sdl.SDL_ReleaseGPUTransferBuffer(device, transfer_buf);

    const transfer = sdl.SDL_MapGPUTransferBuffer(
        device,
        transfer_buf,
        false,
    ) orelse return error.SDL_MapGPUTransferBuffer;
    defer sdl.SDL_UnmapGPUTransferBuffer(device, transfer_buf);

    const data_slice = @as([*]u8, @ptrCast(item.data))[0..item.data.len];
    const transfer_slice = @as([*]u8, @ptrCast(transfer))[0..item.data.len];
    std.mem.copyForwards(u8, transfer_slice, data_slice);

    switch (item.location) {
        .buf => |buffer| {
            sdl.SDL_UploadToGPUBuffer(
                pass,
                &.{ .transfer_buffer = transfer_buf },
                &.{ .buffer = buffer },
                false,
            );
        },
        .tex => |info| {
            sdl.SDL_UploadToGPUTexture(
                pass,
                &.{
                    .transfer_buffer = transfer_buf,
                    .pixels_per_row = info.width,
                    .rows_per_layer = info.height,
                },
                &.{
                    .texture = info.texture,
                    .w = info.width,
                    .h = info.height,
                    // default depth of a flat 2D texture
                    // FIXME: Handle other texture types
                    .d = 1,
                },
                false,
            );
        },
    }
}

fn pop_front(list: *std.ArrayList(Item)) ?Item {
    if (list.items.len == 0) return null;
    const item = list.orderedRemove(0);
    return item;
}
