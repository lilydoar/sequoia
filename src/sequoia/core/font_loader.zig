const freetype = @import("freetype");

const Self = @This();

lib: freetype.Library,

pub fn init() !Self {
    return Self{
        .lib = try freetype.Library.init(),
    };
}

pub fn deinit(self: Self) void {
    self.lib.deinit();
}

pub fn loadFont(self: Self, path: [:0]const u8) !void {
    const face = try self.lib.createFace(path.ptr, 0);
    _ = face; // autofix
}
