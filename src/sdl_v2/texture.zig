const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

// const TextureDesc = struct {
//     type: sdl.SDL_GPUTextureType ,
//     format: sdl.SDL_GPUTextureFormat ,
//     usage: SDL_GPUTextureUsageFlags = @import("std").mem.zeroes(SDL_GPUTextureUsageFlags),
//     width: Uint32 = @import("std").mem.zeroes(Uint32),
//     height: Uint32 = @import("std").mem.zeroes(Uint32),
//     layer_count_or_depth: Uint32 = @import("std").mem.zeroes(Uint32),
//     num_levels: Uint32 = @import("std").mem.zeroes(Uint32),
//     sample_count: SDL_GPUSampleCount = @import("std").mem.zeroes(SDL_GPUSampleCount),
// };

const Self = @This();

pub fn init(device: *sdl.SDL_GPUDevice) Self {
    const info = sdl.SDL_GPUTextureCreateInfo{};
    sdl.SDL_CreateGPUTexture(device, info);
}
