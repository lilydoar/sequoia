const std = @import("std");

const Runner = @import("hot_reload/runner.zig");
const Watcher = @import("hot_reload/watcher.zig");

const lib_path = "zig-out/lib/libsequoia.dylib";

pub fn main() !void {
    var runner = try Runner.init(lib_path);
    var watcher = try Watcher.init(lib_path);

    var running = true;
    while (running) {
        if (try watcher.isModified()) {
            try runner.reload(lib_path);
        }
        running = runner.tick();
        runner.draw();
    }
}
