const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Descriptor = struct {
    pub const Flags = struct {
        fullscreen: bool = false,
        borderless: bool = false,
        resizable: bool = false,
        always_on_top: bool = false,

        fn toUint64(self: Flags) u64 {
            var flags: u64 = 0;
            if (self.fullscreen) flags |= 0x0000000000000001;
            if (self.borderless) flags |= 0x0000000000000010;
            if (self.resizable) flags |= 0x0000000000000020;
            if (self.always_on_top) flags |= 0x0000000000010000;
            return flags;
        }
    };

    title: []const u8,
    width: u32,
    height: u32,
    flags: Flags,
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
        desc.flags.toUint64(),
    ) orelse return error.CreateWindow;

    return Self{
        .descriptor = desc,
        .ptr = window,
    };
}

pub fn deinit(self: Self) void {
    sdl.SDL_DestroyWindow(self.ptr);
}
