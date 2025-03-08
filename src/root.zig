const std = @import("std");

pub const SDL3 = @import("sdl.zig").SDL;

pub export fn newWindow() void {
    std.debug.print("Creating a new window\n", .{});
}
