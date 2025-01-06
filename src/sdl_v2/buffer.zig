const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Desc = struct {
    usage: sdl.SDL_GPUBufferUsageFlags,
    size: u32,
};

pub fn init(device: *sdl.SDL_GPUDevice, desc: Desc) !*sdl.SDL_GPUBuffer {
    return sdl.SDL_CreateGPUBuffer(device, .{
        .usage = desc.usage,
        .size = desc.size,
    }) orelse return error.SDL_CreateGPUBuffer;
}
pub fn deinit(device: *sdl.SDL_GPUDevice, buffer: *sdl.SDL_GPUBuffer) void {
    sdl.SDL_ReleaseGPUBuffer(device, buffer);
}
