const std = @import("std");
const Context = @import("sdl/context.zig");
const tri = @import("render/triangle.zig");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const app_lifetime = gpa.allocator();

    // App init
    const app: Context.App = .{
        .name = "Sequoia",
        .url = "github.com/lilydoar/sequoia",
    };
    const window: Context.Window = .{
        .width = 800,
        .height = 600,
    };
    var context = try Context.init(app_lifetime, app, window);
    defer context.deinit();

    // Init render
    // TODO: Create vertex buffer
    const triangle = try tri.init(&context);

    // Upload static data

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

        // Update Game
        std.debug.assert(context.gpu_upload_staging.staged() == 0);
        {}
        try context.gpu_upload_staging.flush(context.device);

        // Draw Game
        {
            const commandBuf = sdl.SDL_AcquireGPUCommandBuffer(
                context.device,
            ) orelse return error.SDL_AcquireGPUCommandBuffer;
            errdefer _ = sdl.SDL_CancelGPUCommandBuffer(commandBuf);

            try triangle.draw(context, commandBuf, .{
                .primitives = .{
                    .num_vertices = 3,
                    .num_instances = 1,
                },
            });

            if (!sdl.SDL_SubmitGPUCommandBuffer(commandBuf)) {
                return error.SDL_SubmitGPUCommandBuffer;
            }
        }
    }
}
