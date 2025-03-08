const std = @import("std");

const sequoia = @import("sequoia");
const sdl = sequoia.SDL3;

const shader = @import("shader.zig");

pub fn main() !void {
    var DA = std.heap.DebugAllocator(.{}){};
    const allocator = DA.allocator();

    if (!sdl.SDL_Init(sdl.SDL_INIT_VIDEO)) return error.SDL_Init;
    defer sdl.SDL_Quit();

    const window_width = 1080;
    const window_height = 720;
    const window = sdl.SDL_CreateWindow(
        "shape rendering",
        window_width,
        window_height,
        0,
    ) orelse return error.SDL_CreateWindow;
    defer sdl.SDL_DestroyWindow(window);

    const supported_shader_formats =
        sdl.SDL_GPU_SHADERFORMAT_SPIRV |
        sdl.SDL_GPU_SHADERFORMAT_DXIL |
        sdl.SDL_GPU_SHADERFORMAT_MSL;

    const debug = true;
    const device = sdl.SDL_CreateGPUDevice(supported_shader_formats, debug, null) orelse return error.SDL_CreateGPUDevice;
    defer sdl.SDL_DestroyGPUDevice(device);

    const shader_format = sdl.SDL_GPU_SHADERFORMAT_MSL;

    if (!sdl.SDL_ClaimWindowForGPUDevice(device, window)) return error.SDL_ClaimWindowForGPUDevice;
    defer sdl.SDL_ReleaseWindowFromGPUDevice(device, window);

    const transfer_buffer = sdl.SDL_CreateGPUTransferBuffer(device, &sdl.SDL_GPUTransferBufferCreateInfo{
        .usage = sdl.SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = 1024,
    }) orelse return error.SDL_CreateGPUTransferBuffer;
    defer sdl.SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    var time = sdl.SDL_GetTicks();
    var prev_time = time;

    const target_fps = 60;
    const fixed_delta_time = 1000 / target_fps;

    var ticks: u64 = 0;
    var accumulator: u64 = 0;

    var input = Input{};

    // game init
    var game = Game.init(Vec2{ .x = window_width, .y = window_height });
    game.state = Game.State.Playing;
    game.ball.vel = Game.Ball.initial_velocity;

    const vertex_shader_path = try shader.buildShaderPath(
        allocator,
        "examples/pong/shaders/gen",
        "circle",
        sdl.SDL_GPU_SHADERSTAGE_VERTEX,
        shader_format,
    );
    const vertex_shader = try shader.fromFile(allocator, device, vertex_shader_path);
    _ = vertex_shader;

    var running = true;
    while (running) {
        prev_time = time;
        time = sdl.SDL_GetTicks();

        const delta_time = time - prev_time;
        accumulator += delta_time;

        var event: sdl.SDL_Event = undefined;
        while (sdl.SDL_PollEvent(&event)) {
            if (event.type == sdl.SDL_EVENT_QUIT) running = false;
            if (event.type == sdl.SDL_EVENT_KEY_DOWN and event.key.key == sdl.SDLK_ESCAPE) running = false;

            if (event.type == sdl.SDL_EVENT_KEY_DOWN) {
                if (event.key.key == sdl.SDLK_UP) {
                    input.pressing_up = true;
                } else if (event.key.key == sdl.SDLK_DOWN) {
                    input.pressing_down = true;
                } else if (event.key.key == sdl.SDLK_RETURN) {
                    input.pressing_enter = true;
                }
            } else if (event.type == sdl.SDL_EVENT_KEY_UP) {
                if (event.key.key == sdl.SDLK_UP) {
                    input.pressing_up = false;
                } else if (event.key.key == sdl.SDLK_DOWN) {
                    input.pressing_down = false;
                } else if (event.key.key == sdl.SDLK_RETURN) {
                    input.pressing_enter = false;
                }
            }
        }

        // update input

        while (accumulator > fixed_delta_time and running) {
            accumulator -= fixed_delta_time;
            ticks += 1;

            // update physics
            if (game.state == .Playing) {
                game.ball.integrate(fixed_delta_time);
            }

            // update ai
            if (game.state == .Playing) {}

            // update game logic

            // game render
            if (game.state == .Menu) {} else if (game.state == .Playing) {
                // render game
            } else if (game.state == .Player1Win) {
                // render player 1 win
            } else if (game.state == .Player2Win) {
                // render player 2 win
            }

            if (input.pressing_up) std.debug.print("pressing up\n", .{});
            if (input.pressing_down) std.debug.print("pressing down\n", .{});
            if (input.pressing_enter) std.debug.print("pressing enter\n", .{});
            // std.debug.print("ball pos: ({}, {})\n", .{ game.ball.pos.x, game.ball.pos.y });
        }
    }
}

const Game = struct {
    state: State = .Menu,

    ball: Ball,
    player1Paddle: Paddle,
    player2Paddle: Paddle,

    pub fn init(window_size: Vec2) Game {
        const paddle_size = Vec2{ .x = 10, .y = 100 };

        return Game{
            .ball = Ball{
                .pos = Vec2{ .x = window_size.x / 2, .y = window_size.y / 2 },
                .vel = Vec2{ .x = 0, .y = 0 },
            },
            .player1Paddle = Paddle{
                .pos = Vec2{ .x = 0, .y = window_size.y / 2 },
                .size = paddle_size,
            },
            .player2Paddle = Paddle{
                .pos = Vec2{ .x = window_size.x - paddle_size.x, .y = window_size.y / 2 },
                .size = paddle_size,
            },
        };
    }

    const State = enum { Menu, Playing, Player1Win, Player2Win };

    const Ball = struct {
        pos: Vec2,
        vel: Vec2,

        pub const initial_velocity = Vec2{ .x = 200, .y = 200 };

        pub fn integrate(self: *Ball, delta_t: f32) void {
            self.pos.x += self.vel.x * delta_t;
            self.pos.y += self.vel.y * delta_t;
        }
    };

    const Paddle = struct {
        pos: Vec2,
        size: Vec2,
    };
};

const Input = struct {
    pressing_up: bool = false,
    pressing_down: bool = false,
    pressing_enter: bool = false,
};

const Vec2 = struct {
    x: f32,
    y: f32,
};
