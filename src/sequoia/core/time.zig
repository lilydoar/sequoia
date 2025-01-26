const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const NS_PER_MS = 1_000_000;
const NS_TO_MS = 1 / NS_PER_MS;

const NS_PER_SEC = 1_000_000_000;
const NS_TO_SEC = 1 / NS_PER_SEC;

const Self = @This();

prev: u64,
curr: u64,

pub fn init() Self {
    const ticks = sdl.SDL_GetTicksNS();
    return Self{
        .prev = ticks,
        .curr = ticks,
    };
}

pub fn tick(self: Self) Self {
    return Self{
        .prev = self.curr,
        .curr = sdl.SDL_GetTicksNS(),
    };
}

pub fn delta(self: Self) u64 {
    return self.curr - self.prev;
}

// FIXME: lossy conversions
pub fn toMS(from: u64) f64 {
    return @as(f64, @floatFromInt(from)) * NS_TO_MS;
}

pub fn toSec(from: u64) f64 {
    return @as(f64, @floatFromInt(from)) * NS_TO_SEC;
}
