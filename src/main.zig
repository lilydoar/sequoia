const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    // const allocator = gpa.allocator();

    if (!sdl.SDL_InitSubSystem(sdl.SDL_INIT_VIDEO)) {
        std.log.err("{s}\n", .{sdl.SDL_GetError()});
        return;
    }
    defer sdl.SDL_QuitSubSystem(sdl.SDL_INIT_VIDEO);
    defer sdl.SDL_Quit();

    const window: Window = .{
        .title = "test window",
        .width = 600,
        .height = 400,
    };
    const sdl_window = sdl.SDL_CreateWindow(
        window.title.ptr,
        window.width,
        window.height,
        window.flags,
    );
    defer sdl.SDL_DestroyWindow(sdl_window);

    var quit = false;
    while (!quit) {
        var event: sdl.SDL_Event = undefined;
        while (sdl.SDL_PollEvent(&event)) {
            if (event.type == sdl.SDL_EVENT_QUIT) {
                quit = true;
            }
        }
    }
}

const Window = struct {
    title: []const u8,
    width: u32,
    height: u32,
    flags: sdl.SDL_WindowFlags = 0,
};
