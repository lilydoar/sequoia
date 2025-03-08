const std = @import("std");

const sequoia = @import("sequoia");
const SDL = sequoia.SDL3;

const MB = 1024 * 1024;

pub const Bindings = struct {
    samplers: u32,
    storage_textures: u32,
    storage_buffers: u32,
    uniform_buffers: u32,
};

pub fn buildShaderPath(
    allocator: std.mem.Allocator,
    path: []const u8,
    name: []const u8,
    stage: SDL.SDL_GPUShaderStage,
    format: SDL.SDL_GPUShaderFormat,
) ![]const u8 {
    const shader_stage = try strFromStage(stage);
    const shader_format = try strFromFormat(format);
    return try std.fmt.allocPrint(allocator, "{s}/{s}.{s}.{s}", .{ path, name, shader_stage, shader_format });
}

pub fn fromFile(
    allocator: std.mem.Allocator,
    device: *SDL.SDL_GPUDevice,
    path: []const u8,
) !*SDL.SDL_GPUShader {
    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();

    const alloc = arena.allocator();

    // Read shader contents
    const contents = std.fs.cwd().readFileAlloc(alloc, path, MB) catch return error.ReadShaderContents;

    // Read shader bindings
    var split_path = std.mem.splitBackwardsScalar(u8, path, '.');
    const format = split_path.next() orelse return error.InvalidShaderPath;
    const stage = split_path.next() orelse return error.InvalidShaderPath;

    const basename = path[0 .. path.len - format.len - 1];
    const json_path = try std.fmt.allocPrint(alloc, "{s}.{s}", .{ basename, "json" });
    const json_content = std.fs.cwd().readFileAlloc(alloc, json_path, MB) catch return error.ReadShaderBindings;
    const bindings = try std.json.parseFromSliceLeaky(Bindings, alloc, json_content, .{});

    // Create shader
    const shader_stage = stageFromStr(stage) catch return error.InvalidShaderStage;
    const shader_format = formatFromStr(format) catch return error.InvalidShaderFormat;
    const shader_entrypoint = switch (shader_format) {
        SDL.SDL_GPU_SHADERFORMAT_MSL => "main0",
        else => "main",
    };

    return SDL.SDL_CreateGPUShader(
        device,
        &SDL.SDL_GPUShaderCreateInfo{
            .code_size = contents.len,
            .code = contents.ptr,
            .entrypoint = shader_entrypoint,
            .format = shader_format,
            .stage = shader_stage,
            .num_samplers = bindings.samplers,
            .num_storage_textures = bindings.storage_textures,
            .num_storage_buffers = bindings.storage_buffers,
            .num_uniform_buffers = bindings.uniform_buffers,
        },
    ) orelse return error.SDL_CreateGPUShader;
}

pub fn formatFromStr(str: []const u8) !SDL.SDL_GPUShaderFormat {
    return if (strCmp(str, "dxil"))
        SDL.SDL_GPU_SHADERFORMAT_DXIL
    else if (strCmp(str, "msl"))
        SDL.SDL_GPU_SHADERFORMAT_MSL
    else if (strCmp(str, "spv"))
        SDL.SDL_GPU_SHADERFORMAT_SPIRV
    else
        return error.UnsupportedShaderFormat;
}

pub fn stageFromStr(str: []const u8) !SDL.SDL_GPUShaderStage {
    return if (strCmp(str, "vert"))
        @intCast(SDL.SDL_GPU_SHADERSTAGE_VERTEX)
    else if (strCmp(str, "frag"))
        @intCast(SDL.SDL_GPU_SHADERSTAGE_FRAGMENT)
    else
        return error.InvalidShaderStage;
}

fn strCmp(a: []const u8, b: []const u8) bool {
    return std.mem.eql(u8, a, b);
}

pub fn strFromFormat(format: SDL.SDL_GPUShaderFormat) ![]const u8 {
    if (format & SDL.SDL_GPU_SHADERFORMAT_DXIL > 0) return "dxil";
    if (format & SDL.SDL_GPU_SHADERFORMAT_MSL > 0) return "msl";
    if (format & SDL.SDL_GPU_SHADERFORMAT_SPIRV > 0) return "spv";
    return error.UnsupportedShaderFormat;
}

pub fn strFromStage(stage: SDL.SDL_GPUShaderFormat) ![]const u8 {
    return switch (stage) {
        SDL.SDL_GPU_SHADERSTAGE_VERTEX => "vert",
        SDL.SDL_GPU_SHADERSTAGE_FRAGMENT => "frag",
        else => return error.UnsupportedShaderStage,
    };
}
