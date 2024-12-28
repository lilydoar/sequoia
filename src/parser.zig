// parse data about text
//

const Lexer = struct {
    // Read file to an internal buffer
    fn read_file(path: []const u8) !void {

    //     const file = try std.fs.cwd().createFile(
    //     "junk_file.txt",
    //     .{ .read = true },
    // );
    // defer file.close();

    // try file.writeAll("Hello File!");

    var buffer: [100]u8 = undefined;
    try file.seekTo(0);
    const bytes_read = try file.readAll(&buffer);

    try expect(eql(u8, buffer[0..bytes_read], "Hello File!"));

    const file = try std.fs.cwd().createFile(
        "junk_file2.txt",
        .{ .read = true },
    );
    defer file.close();
    const stat = try file.stat();
    try expect(stat.size == 0);
    try expect(stat.kind == .file);
    try expect(stat.ctime <= std.time.nanoTimestamp());
    try expect(stat.mtime <= std.time.nanoTimestamp());
    try expect(stat.atime <= std.time.nanoTimestamp());
    }

};
