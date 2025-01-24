const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const MB = 1024 * 1024;

const Item = struct {
    data: []u8,
    location: union(enum) {
        buf: *sdl.SDL_GPUBuffer,
    },
};

const Self = @This();

items: std.ArrayList(Item),
buf: *sdl.SDL_GPUTransferBuffer,

// TODO: I think I want to turn this transfer_queue struct into just a
// TransferBuffer struct. Will still have a nice API. And will better
// fit the pattern I've been following to wrap SDL

pub fn init(alloc: std.mem.Allocator, device: *sdl.SDL_GPUDevice) !Self {
    return .{
        .items = std.ArrayList(Item).init(alloc),
        .buf = sdl.SDL_CreateGPUTransferBuffer(device, &.{
            .usage = sdl.SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = MB,
        }) orelse return error.SDL_CreateGPUTransferBuffer,
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    self.items.deinit();
    sdl.SDL_ReleaseGPUTransferBuffer(device, self.buf);
}

pub fn stage(self: *Self, item: Item) !void {
    try self.items.append(item);
}

pub fn staged(self: Self) usize {
    return self.items.items.len;
}

pub fn flush(self: *Self, device: *sdl.SDL_GPUDevice) !void {
    const cmds = sdl.SDL_AcquireGPUCommandBuffer(device) orelse
        return error.SDL_AcquireGPUCommandBuffer;
    errdefer _ = sdl.SDL_CancelGPUCommandBuffer(cmds);

    const pass = sdl.SDL_BeginGPUCopyPass(cmds) orelse
        return error.SDL_BeginGPUCopyPass;

    while (self.staged() > 0) {
        try self.uploadItem(device, pass);
    }

    sdl.SDL_EndGPUCopyPass(pass);
    if (!sdl.SDL_SubmitGPUCommandBuffer(cmds)) {
        return error.SDL_SubmitGPUCommandBuffer;
    }
}

fn uploadItem(
    self: *Self,
    device: *sdl.SDL_GPUDevice,
    pass: *sdl.SDL_GPUCopyPass,
) !void {
    const item = popFront(&self.items) orelse return;

    if (item.data.len > MB) {
        return error.DataTooLarge;
    }

    const transfer = sdl.SDL_MapGPUTransferBuffer(
        device,
        self.buf,
        true,
    ) orelse return error.SDL_MapGPUTransferBuffer;
    defer sdl.SDL_UnmapGPUTransferBuffer(device, self.buf);

    std.mem.copyForwards(
        u8,
        @as([*]u8, @ptrCast(transfer))[0..item.data.len],
        item.data,
    );

    switch (item.location) {
        .buf => |buf| {
            sdl.SDL_UploadToGPUBuffer(
                pass,
                &.{ .transfer_buffer = self.buf },
                &.{
                    .buffer = buf,
                    .size = @intCast(item.data.len),
                },
                true,
            );
        },
    }
}

fn popFront(list: *std.ArrayList(Item)) ?Item {
    if (list.items.len == 0) return null;
    const item = list.orderedRemove(0);
    return item;
}
