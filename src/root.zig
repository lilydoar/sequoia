const std = @import("std");

const StateExternal = *anyopaque;
const StateInternal = struct {};

pub export fn init() StateExternal {
    return undefined;
}

pub export fn deinit(state: StateExternal) void {
    _ = state; // autofix
}

pub export fn reload(state: StateExternal) void {
    _ = state; // autofix
    std.debug.print("reload\n", .{});
}

pub export fn tick(state: StateExternal) bool {
    _ = state; // autofix
    std.debug.print("tick\n", .{});
    return false;
}

pub export fn draw(state: StateExternal) void {
    _ = state; // autofix
    std.debug.print("draw\n", .{});
}
