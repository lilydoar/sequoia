const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Descriptor = struct {
    pub const Bindings = struct {
        samplers: u32,
        storage_textures: u32,
        storage_buffers: u32,
        uniform_buffers: u32,
    };

    format: sdl.SDL_GPUShaderFormat,
    stage: sdl.SDL_GPUShaderStage,
    bindings: Bindings,
};

const KB: usize = 1024;

const Self = @This();

desc: Descriptor,
shader: *sdl.SDL_GPUShader,

pub fn fromFile(
    alloc: std.mem.Allocator,
    device: *sdl.SDL_GPUDevice,
    path: []const u8,
) !Self {
    var scope = std.heap.ArenaAllocator.init(alloc);
    defer scope.deinit();
    const allocator = scope.allocator();

    const code = std.fs.cwd().readFileAlloc(allocator, path, KB) catch
        return error.InvalidShaderPath;

    var parts = std.mem.splitBackwardsScalar(u8, path, '.');
    const format_str = parts.next() orelse return error.InvalidShaderPath;
    const stage_str = parts.next() orelse return error.InvalidShaderPath;

    const format = if (strCmp(format_str, "dxil"))
        sdl.SDL_GPU_SHADERFORMAT_DXIL
    else if (strCmp(format_str, "msl"))
        sdl.SDL_GPU_SHADERFORMAT_MSL
    else if (strCmp(format_str, "spv"))
        sdl.SDL_GPU_SHADERFORMAT_SPIRV
    else
        return error.InvalidShaderPath;

    const stage = if (strCmp(stage_str, "vert"))
        sdl.SDL_GPU_SHADERSTAGE_VERTEX
    else if (strCmp(stage_str, "frag"))
        sdl.SDL_GPU_SHADERSTAGE_FRAGMENT
    else
        return error.InvalidShaderPath;

    const json_path = try std.fmt.allocPrint(allocator, "{s}.{s}", .{
        path[0 .. path.len - format_str.len - 1],
        "json",
    });
    const json_content = std.fs.cwd().readFileAlloc(
        allocator,
        json_path,
        KB,
    ) catch return error.InvalidShaderPath;
    const bindings = try std.json.parseFromSliceLeaky(
        Descriptor.Bindings,
        allocator,
        json_content,
        .{},
    );

    const desc = Descriptor{
        .format = format,
        .stage = @intCast(stage),
        .bindings = bindings,
    };

    const shader = sdl.SDL_CreateGPUShader(device, &.{
        .code = code.ptr,
        .code_size = code.len,
        .entrypoint = switch (desc.format) {
            sdl.SDL_GPU_SHADERFORMAT_MSL => "main0",
            else => "main",
        },
        .format = desc.format,
        .stage = desc.stage,
        .num_samplers = desc.bindings.samplers,
        .num_storage_textures = desc.bindings.storage_textures,
        .num_storage_buffers = desc.bindings.storage_buffers,
        .num_uniform_buffers = desc.bindings.uniform_buffers,
    }) orelse return error.CreateShader;

    return Self{
        .desc = desc,
        .shader = shader,
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUShader(device, self.shader);
}

fn strCmp(a: []const u8, b: []const u8) bool {
    return std.mem.eql(u8, a, b);
}
