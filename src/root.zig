const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Context = @import("sequoia/core/context.zig");
const App = @import("sequoia/core/app.zig");
const Window = @import("sequoia/core/window.zig");
const Device = @import("sequoia/core/device.zig");

const Shader = @import("sequoia/render/shader.zig");
const VertexBuffer = @import("sequoia/render/vertex_buffer.zig");
const IndexBuffer = @import("sequoia/render/index_buffer.zig");
const Pipeline = @import("sequoia/render/pipeline.zig");
const TransferQueue = @import("sequoia/render/transfer_queue.zig");
const Vertex = @import("sequoia/render/vertices.zig").ColoredVertex;

const MB = 1024 * 1024;

const StateOpaque = *anyopaque;

const State = struct {
    static_alloc: std.heap.GeneralPurposeAllocator(.{}),
    frame_alloc: std.heap.ArenaAllocator,
    rng: std.Random.DefaultPrng,
    ctx: Context,

    pipeline: Pipeline,
    transfer_queue: TransferQueue,
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

    state.transfer_queue.deinit(state.ctx.device.ptr);
    state.pipeline.deinit(state.ctx.device.ptr);
    state.ctx.deinit();

    state.frame_alloc.deinit();
    _ = state.static_alloc.deinit();
}

pub export fn reload(state_opaque: StateOpaque) void {
    const state = fromOpaquePtr(state_opaque);

    gameReload(state) catch |err| {
        std.debug.print("Failed to reload game: {any}\n", .{err});
    };
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

    gameDraw(state) catch |err| {
        std.debug.print("Failed to draw game: {any}\n", .{err});
    };
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
            .flags = .{
                .resizable = true,
                .always_on_top = true,
                .borderless = false,
            },
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
    const vert_buf = try VertexBuffer.init(
        scope_alloc,
        ctx.device.ptr,
        @sizeOf(Vertex) * 4,
        &.{
            .{
                .size = @sizeOf(f32) * 2,
                .format = sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            .{
                .size = @sizeOf(f32) * 4,
                .format = sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            },
        },
    );
    const idx_buf = try IndexBuffer.init(ctx.device.ptr, .{
        .capacity = @sizeOf(f16) * 6,
    });
    var pipeline = try Pipeline.init(
        static_alloc,
        scope_alloc,
        ctx.device.ptr,
        .{
            .target = .{
                .color_target_descriptions = &.{
                    .format = sdl.SDL_GetGPUSwapchainTextureFormat(
                        ctx.device.ptr,
                        ctx.window.ptr,
                    ),
                },
                .num_color_targets = 1,
            },
        },
        vert_shader,
        frag_shader,
        &.{vert_buf},
        idx_buf,
    );

    var queue = try TransferQueue.init(
        static_alloc,
        ctx.device.ptr,
        .{ .capacity = MB },
    );

    try loadStaticData(ctx.device, &pipeline, &queue);

    const state = try static_alloc.create(State);
    state.static_alloc = gpa;
    state.frame_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    state.rng = rng;
    state.ctx = ctx;

    state.pipeline = pipeline;
    state.transfer_queue = queue;

    return state;
}

fn gameReload(state: *State) !void {
    try loadStaticData(
        state.ctx.device,
        &state.pipeline,
        &state.transfer_queue,
    );
}

fn gameDraw(state: *State) !void {
    const cmd_buf = sdl.SDL_AcquireGPUCommandBuffer(state.ctx.device.ptr) orelse
        return error.AcquireGPUCommandBuffer;
    errdefer _ = sdl.SDL_CancelGPUCommandBuffer(cmd_buf);

    var swapchain: ?*sdl.SDL_GPUTexture = null;
    if (!sdl.SDL_WaitAndAcquireGPUSwapchainTexture(
        cmd_buf,
        state.ctx.window.ptr,
        &swapchain,
        null,
        null,
    )) return error.WaitAndAcquireGPUSwapchainTexture;

    const pass = sdl.SDL_BeginGPURenderPass(
        cmd_buf,
        &.{
            .texture = swapchain,
            .clear_color = .{ .r = 0, .g = 0, .b = 0, .a = 1 },
            .load_op = sdl.SDL_GPU_LOADOP_CLEAR,
            .store_op = sdl.SDL_GPU_STOREOP_STORE,
        },
        1,
        null,
    ) orelse return error.BeginGPURenderPass;

    state.pipeline.bind(pass);
    state.pipeline.draw(pass);

    sdl.SDL_EndGPURenderPass(pass);
    if (!sdl.SDL_SubmitGPUCommandBuffer(cmd_buf))
        return error.SubmitGPUCommandBuffer;
}

fn appInit() !App {
    var app = try App.init("Sequoia");
    try app.withVersion("0.1.0");
    try app.withIdentifier("com.sequoia");
    return app;
}

fn loadStaticData(device: Device, pipeline: *Pipeline, queue: *TransferQueue) !void {
    var vertices = [_]Vertex{
        .{ .pos = .{ -1, -1 }, .color = .{ 0, 1, 0, 1 } },
        .{ .pos = .{ 0, 1 }, .color = .{ 0, 1, 0, 1 } },
        .{ .pos = .{ 0.72, -0.24 }, .color = .{ 1, 0, 1, 1 } },
        .{ .pos = .{ -0.8, 0.8 }, .color = .{ 1, 1, 1, 1 } },
    };
    try pipeline.vert_bufs.items[0].upload(queue, std.mem.sliceAsBytes(&vertices));

    var indices = [_]u16{ 0, 1, 2, 0, 3, 1 };
    try pipeline.idx_buf.upload(queue, std.mem.sliceAsBytes(&indices));

    try queue.flush(device.ptr);
}

fn fromOpaquePtr(ptr: *anyopaque) *State {
    return @ptrCast(@alignCast(ptr));
}
