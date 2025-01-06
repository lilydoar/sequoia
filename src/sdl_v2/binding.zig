const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Desc = struct {
    slot: u32 = 0,
    offset: u32 = 0,
    resource: union(enum) {
        buffer: *sdl.SDL_GPUBuffer,
        texture: *sdl.SDL_GPUTexture,
        sampled_texture: struct {
            texture: *sdl.SDL_GPUTexture,
            sampler: *sdl.SDL_GPUSampler,
        },
    },
};
