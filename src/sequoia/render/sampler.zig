const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Descriptor = struct {
    min_filter: sdl.SDL_GPUFilter,
    mag_filter: sdl.SDL_GPUFilter,
    address_mode: sdl.SDL_GPUSamplerAddressMode,
};

const Self = @This();

desc: Descriptor,
ptr: *sdl.SDL_GPUSampler,

pub fn init(device: *sdl.SDL_GPUDevice, desc: Descriptor) !Self {
    const sampler = sdl.SDL_CreateGPUSampler(device, &.{
        .min_filter = desc.min_filter,
        .mag_filter = desc.mag_filter,
        .address_mode_u = desc.address_mode,
        .address_mode_v = desc.address_mode,
        .address_mode_w = desc.address_mode,
    }) orelse return error.SDL_CreateGPUSampler;

    return Self{
        .desc = desc,
        .ptr = sampler,
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUSampler(device, self.ptr);
}
