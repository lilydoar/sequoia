const std = @import("std");

const Runner = @import("hot_reload/runner.zig");
const Watcher = @import("hot_reload/watcher.zig");

const lib_path = "zig-out/lib/libgame.dylib";

pub fn main() !void {
    var runner = try Runner.init(lib_path);
    var watcher = try Watcher.init(lib_path);

    var quit = false;
    while (!quit) {
        if (try watcher.isModified()) {
            try runner.reload(lib_path);
        }
        quit = runner.tick();
        runner.draw();
    }
}
