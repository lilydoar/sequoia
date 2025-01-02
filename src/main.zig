const std = @import("std");
const sdl_internal = @import("sdl.zig");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    // const allocator = gpa.allocator();

    const app: sdl_internal.App = .{
        .name = "Sequoia",
        .url = "github.com/lilydoar/sequoia",
    };
    const window: sdl_internal.Window = .{
        .width = 800,
        .height = 600,
    };
    const sdl_context = try sdl_internal.Context.init(app, window);
    defer sdl_context.deinit();

    var quit = false;
    while (!quit) {
        var event: sdl.SDL_Event = undefined;
        while (sdl.SDL_PollEvent(&event)) {
            if (event.type == sdl.SDL_EVENT_QUIT) {
                quit = true;
            }
            if (event.type == sdl.SDL_EVENT_KEY_DOWN and
                event.key.key == sdl.SDLK_ESCAPE)
            {
                quit = true;
            }
        }

        // Game state

        // Upload state to GPU

        // Draw game
    }
}
