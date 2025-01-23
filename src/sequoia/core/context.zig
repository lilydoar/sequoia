const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const App = @import("app.zig");
const Window = @import("window.zig");
const Device = @import("device.zig");

const Self = @This();

app: App,
window: Window,
device: Device,

pub fn init(app: App, window_desc: Window.Descriptor) !Self {
    const window = try Window.init(window_desc);
    const device = try Device.init(false);

    if (!sdl.SDL_ClaimWindowForGPUDevice(device.ptr, window.ptr))
        return error.ClaimWindow;

    return Self{
        .app = app,
        .window = window,
        .device = device,
    };
}

pub fn deinit(self: Self) void {
    sdl.SDL_ReleaseWindowFromGPUDevice(self.device.ptr, self.window.ptr);
    self.device.deinit();
    self.window.deinit();
    sdl.SDL_Quit();
}
