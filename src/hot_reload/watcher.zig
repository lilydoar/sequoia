const std = @import("std");

const Self = @This();

path: []const u8,
last_modified_time: i128,

pub fn init(path: []const u8) !Self {
    const stat = try std.fs.cwd().statFile(path);
    return Self{
        .path = path,
        .last_modified_time = stat.mtime,
    };
}

pub fn isModified(self: *Self) !bool {
    const stat = try std.fs.cwd().statFile(self.path);
    if (stat.mtime <= self.last_modified_time) return false;
    self.last_modified_time = stat.mtime;
    return true;
}
