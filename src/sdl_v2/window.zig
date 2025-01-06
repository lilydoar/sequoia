const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

width: usize,
height: usize,
flags: sdl.SDL_WindowFlags = 0,
