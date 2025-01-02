const std = @import("std");
const ctx = @import("sdl/context.zig");
const shader = @import("sdl/shader.zig");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const app_lifetime = gpa.allocator();

    // App init
    const app: ctx.App = .{
        .name = "Sequoia",
        .url = "github.com/lilydoar/sequoia",
    };
    const window: ctx.Window = .{
        .width = 800,
        .height = 600,
    };
    var context = try ctx.init(app_lifetime, app, window);
    defer context.deinit();

    // Load shaders

    const tri_vert_shader = shader.fromFile("assets/gen/shaders/metal/triangle.metal");

    // const info: sdl.SDL_GPUGraphicsPipelineCreateInfo = {};
    // const tri_pipeline: sdl.SDL_GPUGraphicsPipeline =
    //     sdl.SDL_CreateGPUGraphicsPipeline(context.device, info);
    // _ = tri_pipeline;

    // Game init

    // Init draw

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
        {}
    }
}
