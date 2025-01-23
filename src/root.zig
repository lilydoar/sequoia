const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Context = @import("sequoia/core/context.zig");
const App = @import("sequoia/core/app.zig");
const Window = @import("sequoia/core/window.zig");

const Shader = @import("sequoia/render/shader.zig");

const StateOpaque = *anyopaque;

const State = struct {
    static_alloc: std.heap.GeneralPurposeAllocator(.{}),
    frame_alloc: std.heap.ArenaAllocator,
    rng: std.Random.DefaultPrng,
    ctx: Context,

    vert_shader: Shader,
    frag_shader: Shader,
};

pub export fn init() ?StateOpaque {
    const state = gameInit() catch |err| {
        std.debug.print("Failed to initialize game: {any}\n", .{err});
        return null;
    };
    return @ptrCast(state);
}

pub export fn deinit(state_opaque: StateOpaque) void {
    const state = fromOpaquePtr(state_opaque);

    state.vert_shader.deinit();
    state.frag_shader.deinit();

    state.ctx.deinit();

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

fn gameInit() !*State {
    var scope = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer scope.deinit();
    const scope_alloc = scope.allocator();

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const static_alloc = gpa.allocator();

    const rng = std.Random.DefaultPrng.init(blk: {
        var seed: u64 = undefined;
        try std.posix.getrandom(std.mem.asBytes(&seed));
        break :blk seed;
    });

    const ctx = try Context.init(
        try appInit(),
        Window.Descriptor{
            .title = "Sequoia",
            .width = 600,
            .height = 400,
        },
    );

    const vert_shader = try Shader.fromFile(
        scope_alloc,
        ctx.device.ptr,
        try Shader.buildShaderPath(
            scope_alloc,
            "gen/shaders",
            "identity",
            sdl.SDL_GPU_SHADERSTAGE_VERTEX,
            ctx.device.format,
        ),
    );

    const frag_shader = try Shader.fromFile(
        scope_alloc,
        ctx.device.ptr,
        try Shader.buildShaderPath(
            scope_alloc,
            "gen/shaders",
            "identity",
            sdl.SDL_GPU_SHADERSTAGE_FRAGMENT,
            ctx.device.format,
        ),
    );

    const state = try static_alloc.create(State);
    state.static_alloc = gpa;
    state.frame_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    state.rng = rng;
    state.ctx = ctx;
    state.vert_shader = vert_shader;
    state.frag_shader = frag_shader;

    return state;
}

fn appInit() !App {
    var app = try App.init("Sequoia");
    try app.withVersion("0.1.0");
    try app.withIdentifier("com.sequoia");
    return app;
}
