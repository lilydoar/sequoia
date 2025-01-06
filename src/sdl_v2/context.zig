const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const App = @import("app.zig");
const Window = @import("window.zig");
const Transfer = @import("transfer.zig");
const Pipeline = @import("pipeline.zig");

const Self = @This();

window: *sdl.SDL_Window,
device: *sdl.SDL_GPUDevice,
pipelines: std.ArrayList(*sdl.SDL_GPUGraphicsPipeline),
transfer: Transfer,

pub fn init(alloc: std.mem.Allocator, app: App, window: Window) !Self {
    try app.set_metadata();

    if (!sdl.SDL_InitSubSystem(sdl.SDL_INIT_VIDEO)) {
        return error.SDL_Init_Video;
    }

    const win = sdl.SDL_CreateWindow(
        app.name.ptr,
        @intCast(window.width),
        @intCast(window.height),
        window.flags,
    ) orelse return error.SDL_CreateWindow;

    const device = sdl.SDL_CreateGPUDevice(
        sdl.SDL_GPU_SHADERFORMAT_DXIL |
            sdl.SDL_GPU_SHADERFORMAT_MSL |
            sdl.SDL_GPU_SHADERFORMAT_SPIRV,
        true,
        null,
    ) orelse return error.SDL_CreateGPUDevice;

    if (!sdl.SDL_ClaimWindowForGPUDevice(device, win)) {
        return error.SDL_ClaimWindowForGPUDevice;
    }

    return .{
        .window = win,
        .device = device,
        .shaders = std.ArrayList(*sdl.SDL_GPUShader).init(alloc),
        .pipelines = std.ArrayList(*sdl.SDL_GPUGraphicsPipeline).init(alloc),
        .transfer = Transfer.init(alloc),
    };
}

pub fn deinit(self: *Self) void {
    for (self.pipelines.resources.items) |p| {
        Pipeline.deinit(self.device, p);
    }
    self.pipelines.deinit();

    self.gpu_upload_staging.deinit();
    sdl.SDL_DestroyGPUDevice(self.device);
    sdl.SDL_DestroyWindow(self.window);
    sdl.SDL_Quit();
}
