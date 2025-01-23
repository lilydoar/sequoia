const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Descriptor = struct {
    title: []const u8,
    width: u32,
    height: u32,
    flags: u64 = 0,
};

const Self = @This();

descriptor: Descriptor,
ptr: *sdl.SDL_Window,

pub fn init(desc: Descriptor) !Self {
    if (!sdl.SDL_Init(sdl.SDL_INIT_VIDEO))
        return error.InitVideo;

    const window = sdl.SDL_CreateWindow(
        desc.title.ptr,
        @intCast(desc.width),
        @intCast(desc.height),
        desc.flags,
    ) orelse return error.CreateWindow;

    return Self{
        .descriptor = desc,
        .ptr = window,
    };
}

pub fn deinit(self: Self) void {
    sdl.SDL_DestroyWindow(self.ptr);
}
