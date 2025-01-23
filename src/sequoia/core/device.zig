const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Self = @This();

const app_formats =
    sdl.SDL_GPU_SHADERFORMAT_DXIL |
    sdl.SDL_GPU_SHADERFORMAT_MSL |
    sdl.SDL_GPU_SHADERFORMAT_SPIRV;

ptr: *sdl.SDL_GPUDevice,
format: sdl.SDL_GPUShaderFormat,

pub fn init(debug: bool) !Self {
    const device = sdl.SDL_CreateGPUDevice(
        app_formats,
        debug,
        null,
    ) orelse return error.CreateDevice;

    const device_formats = sdl.SDL_GetGPUShaderFormats(device);

    const format = if (device_formats & sdl.SDL_GPU_SHADERFORMAT_DXIL > 0)
        sdl.SDL_GPU_SHADERFORMAT_DXIL
    else if (device_formats & sdl.SDL_GPU_SHADERFORMAT_MSL > 0)
        sdl.SDL_GPU_SHADERFORMAT_MSL
    else if (device_formats & sdl.SDL_GPU_SHADERFORMAT_SPIRV > 0)
        sdl.SDL_GPU_SHADERFORMAT_SPIRV
    else
        return error.UnsupportedShaderFormat;

    return Self{
        .ptr = device,
        .format = format,
    };
}

pub fn deinit(self: Self) void {
    sdl.SDL_DestroyGPUDevice(self.ptr);
}
