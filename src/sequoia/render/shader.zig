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
ptr: *sdl.SDL_GPUShader,

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
    const format = parts.next() orelse return error.InvalidShaderPath;
    const stage = parts.next() orelse return error.InvalidShaderPath;

    const json_path = try std.fmt.allocPrint(allocator, "{s}.{s}", .{
        path[0 .. path.len - format.len - 1],
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
        .format = try formatFromStr(format),
        .stage = try stageFromStr(stage),
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
        .ptr = shader,
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUShader(device, self.ptr);
}

pub fn formatFromStr(str: []const u8) !sdl.SDL_GPUShaderFormat {
    return if (strCmp(str, "dxil"))
        sdl.SDL_GPU_SHADERFORMAT_DXIL
    else if (strCmp(str, "msl"))
        sdl.SDL_GPU_SHADERFORMAT_MSL
    else if (strCmp(str, "spv"))
        sdl.SDL_GPU_SHADERFORMAT_SPIRV
    else
        return error.InvalidShaderExtension;
}

pub fn strFromFormat(format: sdl.SDL_GPUShaderFormat) ![]const u8 {
    return switch (format) {
        sdl.SDL_GPU_SHADERFORMAT_DXIL => "dxil",
        sdl.SDL_GPU_SHADERFORMAT_MSL => "msl",
        sdl.SDL_GPU_SHADERFORMAT_SPIRV => "spv",
        else => return error.InvalidShaderFormat,
    };
}

pub fn stageFromStr(str: []const u8) !sdl.SDL_GPUShaderStage {
    return if (strCmp(str, "vert"))
        @intCast(sdl.SDL_GPU_SHADERSTAGE_VERTEX)
    else if (strCmp(str, "frag"))
        @intCast(sdl.SDL_GPU_SHADERSTAGE_FRAGMENT)
    else
        return error.InvalidShaderExtension;
}

pub fn strFromStage(stage: sdl.SDL_GPUShaderStage) ![]const u8 {
    return switch (stage) {
        sdl.SDL_GPU_SHADERSTAGE_VERTEX => "vert",
        sdl.SDL_GPU_SHADERSTAGE_FRAGMENT => "frag",
        else => return error.InvalidShaderStage,
    };
}

pub fn buildShaderPath(
    alloc: std.mem.Allocator,
    base_path: []const u8,
    name: []const u8,
    stage: sdl.SDL_GPUShaderStage,
    format: sdl.SDL_GPUShaderFormat,
) ![]const u8 {
    return try std.fmt.allocPrint(alloc, "{s}/{s}.{s}.{s}", .{
        base_path,
        name,
        try strFromStage(stage),
        try strFromFormat(format),
    });
}

fn strCmp(a: []const u8, b: []const u8) bool {
    return std.mem.eql(u8, a, b);
}
