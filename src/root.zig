const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const OpaqueState = @import("hot_reload/opaque_state.zig").OpaqueState;

const Context = @import("sequoia/core/context.zig");
const App = @import("sequoia/core/app.zig");
const Window = @import("sequoia/core/window.zig");
const Device = @import("sequoia/core/device.zig");

const Shader = @import("sequoia/render/shader.zig");
const VertexBuffer = @import("sequoia/render/vertex_buffer.zig");
const IndexBuffer = @import("sequoia/render/index_buffer.zig");
const Pipeline = @import("sequoia/render/pipeline.zig");
const TransferBuffer = @import("sequoia/render/transfer_buffer.zig");
const Vertex = @import("sequoia/render/vertices.zig").ColoredVertex;

const MB = 1024 * 1024;

const State = struct {
    static_alloc: std.heap.GeneralPurposeAllocator(.{}),
    frame_alloc: std.heap.ArenaAllocator,
    rng: std.Random.DefaultPrng,
    ctx: Context,

    transfer_buf: TransferBuffer,
    static_pipeline: Pipeline,
    dynamic_pipeline: Pipeline,
};

pub export fn init(opaque_state: *OpaqueState) void {
    const state = gameInit() catch |err| {
        std.debug.print("Failed to initialize game: {any}\n", .{err});
        return;
    };

    opaque_state.ptr = @ptrCast(state);
    opaque_state.size = @sizeOf(State);
}

pub export fn deinit(opaque_state: *OpaqueState) void {
    const state = opaque_state.toState(State);

    state.transfer_buf.deinit(state.ctx.device.ptr);
    state.static_pipeline.deinit(state.ctx.device.ptr);
    state.dynamic_pipeline.deinit(state.ctx.device.ptr);
    state.ctx.deinit();

    state.frame_alloc.deinit();
    _ = state.static_alloc.deinit();
}

pub export fn reload(opaque_state: *OpaqueState) void {
    const state = opaque_state.toState(State);

    gameReload(state) catch |err| {
        std.debug.print("Failed to reload game: {any}\n", .{err});
    };
}

pub export fn tick(opaque_state: *OpaqueState) bool {
    const state = opaque_state.toState(State);

    return gameTick(state) catch |err| {
        std.debug.print("Failed to tick game: {any}\n", .{err});
        return false;
    };
}

pub export fn draw(opaque_state: *OpaqueState) void {
    const state = opaque_state.toState(State);

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

    var transfer_buf = try TransferBuffer.init(
        static_alloc,
        ctx.device.ptr,
        .{ .capacity = MB },
    );

    var static_pipeline = try Pipeline.init(
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
        try Shader.fromFile(
            scope_alloc,
            ctx.device.ptr,
            try Shader.buildShaderPath(
                scope_alloc,
                "gen/shaders",
                "identity",
                sdl.SDL_GPU_SHADERSTAGE_VERTEX,
                ctx.device.format,
            ),
        ),
        try Shader.fromFile(
            scope_alloc,
            ctx.device.ptr,
            try Shader.buildShaderPath(
                scope_alloc,
                "gen/shaders",
                "identity",
                sdl.SDL_GPU_SHADERSTAGE_FRAGMENT,
                ctx.device.format,
            ),
        ),
        &.{try VertexBuffer.init(
            scope_alloc,
            ctx.device.ptr,
            @sizeOf(Vertex) * 4,
            &.{
                sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            },
        )},
        try IndexBuffer.init(ctx.device.ptr, .{
            .capacity = @sizeOf(f16) * 6,
        }),
    );

    try loadStaticData(ctx.device, &static_pipeline, &transfer_buf);

    const dynamic_pipeline = try Pipeline.init(
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
        try Shader.fromFile(
            scope_alloc,
            ctx.device.ptr,
            try Shader.buildShaderPath(
                scope_alloc,
                "gen/shaders",
                "identity",
                sdl.SDL_GPU_SHADERSTAGE_VERTEX,
                ctx.device.format,
            ),
        ),
        try Shader.fromFile(
            scope_alloc,
            ctx.device.ptr,
            try Shader.buildShaderPath(
                scope_alloc,
                "gen/shaders",
                "identity",
                sdl.SDL_GPU_SHADERSTAGE_FRAGMENT,
                ctx.device.format,
            ),
        ),
        &.{try VertexBuffer.init(
            scope_alloc,
            ctx.device.ptr,
            @sizeOf(Vertex) * 4,
            &.{
                sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            },
        )},
        try IndexBuffer.init(ctx.device.ptr, .{
            .capacity = @sizeOf(f16) * 6,
        }),
    );
    const state = try static_alloc.create(State);
    state.static_alloc = gpa;
    state.frame_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    state.rng = rng;
    state.ctx = ctx;

    state.transfer_buf = transfer_buf;
    state.static_pipeline = static_pipeline;
    state.dynamic_pipeline = dynamic_pipeline;

    return state;
}

fn gameReload(state: *State) !void {
    try loadStaticData(
        state.ctx.device,
        &state.static_pipeline,
        &state.transfer_buf,
    );
}

fn gameTick(state: *State) !bool {
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

    try loadDynamicData(
        state.ctx.device,
        &state.dynamic_pipeline,
        &state.transfer_buf,
    );

    return true;
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

    state.static_pipeline.bind(pass);
    state.static_pipeline.draw(pass);

    state.dynamic_pipeline.bind(pass);
    state.dynamic_pipeline.draw(pass);

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

fn loadStaticData(
    device: Device,
    pipeline: *Pipeline,
    transfer_buf: *TransferBuffer,
) !void {
    var vertices = [_]Vertex{
        .{ .pos = .{ -0.95, -0.95 }, .color = .{ 0, 1, 0, 1 } },
        .{ .pos = .{ 0, 0.98 }, .color = .{ 0, 1, 0, 1 } },
        .{ .pos = .{ 0.72, -0.24 }, .color = .{ 1, 0, 1, 1 } },
        .{ .pos = .{ -0.8, 0.8 }, .color = .{ 1, 1, 1, 1 } },
    };
    try pipeline.vert_bufs.items[0].upload(transfer_buf, std.mem.sliceAsBytes(&vertices));

    var indices = [_]u16{ 0, 1, 2, 0, 3, 1 };
    try pipeline.idx_buf.upload(transfer_buf, std.mem.sliceAsBytes(&indices));

    try transfer_buf.flush(device.ptr);
}

fn loadDynamicData(
    device: Device,
    pipeline: *Pipeline,
    transfer_buf: *TransferBuffer,
) !void {
    var vertices = [_]Vertex{
        .{ .pos = .{ -0.2, -0.8 }, .color = .{ 0.4, 1, 0, 1 } },
        .{ .pos = .{ 0.4, 0.2 }, .color = .{ 0, 1, 0.8, 1 } },
        .{ .pos = .{ 0.65, -0.46 }, .color = .{ 0.2, 0, 1, 1 } },
        .{ .pos = .{ -0.7, 0.5 }, .color = .{ 1, 0.5, 1, 1 } },
    };
    try pipeline.vert_bufs.items[0].upload(transfer_buf, std.mem.sliceAsBytes(&vertices));

    var indices = [_]u16{ 0, 1, 2, 0, 3, 1 };
    try pipeline.idx_buf.upload(transfer_buf, std.mem.sliceAsBytes(&indices));

    try transfer_buf.flush(device.ptr);
}

fn fromOpaquePtr(ptr: *anyopaque) *State {
    return @ptrCast(@alignCast(ptr));
}
