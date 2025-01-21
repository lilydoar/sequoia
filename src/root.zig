const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const SDLContext = @import("sequoia/core/context.zig");
const App = @import("sequoia/core/app.zig");
const Window = @import("sequoia/core/window.zig");

const StateOpaque = *anyopaque;

const State = struct {
    static_alloc: std.heap.GeneralPurposeAllocator(.{}),
    frame_alloc: std.heap.ArenaAllocator,
    rng: std.Random.DefaultPrng,
    sdl_ctx: SDLContext,
};

pub export fn init() ?StateOpaque {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const static_allocator = gpa.allocator();

    const rng = std.Random.DefaultPrng.init(blk: {
        var seed: u64 = undefined;
        std.posix.getrandom(std.mem.asBytes(&seed)) catch return null;
        break :blk seed;
    });

    const sdlCtx = SDLContext.init(
        appInit() catch return null,
        Window.Descriptor{
            .title = "Sequoia",
            .width = 600,
            .height = 400,
        },
    ) catch return null;

    const state = static_allocator.create(State) catch return null;
    state.static_alloc = gpa;
    state.frame_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    state.rng = rng;
    state.sdl_ctx = sdlCtx;

    return @ptrCast(state);
}

pub export fn deinit(state_opaque: StateOpaque) void {
    const state = fromOpaquePtr(state_opaque);

    state.sdl_ctx.deinit();

    state.frame_alloc.deinit();
    _ = state.static_alloc.deinit();
}

pub export fn reload(state_opaque: StateOpaque) void {
    const state = fromOpaquePtr(state_opaque);
    _ = state; // autofix
}

pub export fn tick(state_opaque: StateOpaque) bool {
    const state = fromOpaquePtr(state_opaque);
    _ = state; // autofix

    var event: sdl.SDL_Event = undefined;
    while (sdl.SDL_PollEvent(&event)) {
        if (event.type == sdl.SDL_EVENT_QUIT) {
            return false;
        }
        if (event.type == sdl.SDL_EVENT_KEY_DOWN and
            event.key.key == sdl.SDLK_ESCAPE)
        {
            return false;
        }
    }

    return true;
}

pub export fn draw(state_opaque: StateOpaque) void {
    const state = fromOpaquePtr(state_opaque);
    _ = state; // autofix
}

fn fromOpaquePtr(ptr: *anyopaque) *State {
    return @ptrCast(@alignCast(ptr));
}

fn appInit() !App {
    var app = try App.init("Sequoia");
    try app.withVersion("0.1.0");
    try app.withIdentifier("com.sequoia");
    return app;
}
