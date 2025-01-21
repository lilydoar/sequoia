const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const App = @import("app.zig");
const Window = @import("window.zig");

const Self = @This();

app: App,
window: Window,

pub fn init(app: App, window_desc: Window.Descriptor) !Self {
    return Self{
        .app = app,
        .window = try Window.init(window_desc),
    };
}

pub fn deinit(self: Self) void {
    self.window.deinit();
    sdl.SDL_Quit();
}
