const std = @import("std");
const DynLib = std.DynLib;

const State = *anyopaque;
const Result = enum { succ, fail, cont };

const Context = struct {
    dll_path: []const u8,
    dll: std.DynLib,

    state: State,
    init_fn: *const fn () Result,
    tick_fn: *const fn (State) Result,
    draw_fn: *const fn (State) Result,
    reload_fn: *const fn (State) Result,

    const Self = @This();

    fn load_dll(self: Self) !void {
        if (self.dll != null) return;

        var dll = try DynLib.open(self.dll_path);

        self.dll = dll;
        self.init_fn = dll.lookup(@TypeOf(self.init_fn), "init") orelse
            return error.LookupFailure;
        self.reload_fn = dll.lookup(@TypeOf(self.reload_fn), "reload") orelse
            return error.LookupFailure;
        self.tick_fn = dll.lookup(@TypeOf(self.tick_fn), "tick") orelse
            return error.LookupFailure;
        self.draw_fn = dll.lookup(@TypeOf(self.draw_fn), "draw") orelse
            return error.LookupFailure;
    }

    fn unload_dll(self: Self) void {
        if (self.dll == null) return;

        self.dll.?.close();
    }
};

pub fn main() !void {
    const dll_path = "zig-out/lib/liblib.dylib";
    const context = try Context.init(dll_path);
    _ = context; // autofix
    _ = context; // autofix
    _ = context; // autofix
    _ = context; // autofix
    _ = context; // autofix
}
