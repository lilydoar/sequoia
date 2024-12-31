// parse data about text
//

const std = @import("std");
const Allocator = std.mem.Allocator;
const ArrayList = std.ArrayList;

const Lexer = struct {
    alloc: Allocator,
    buffer: []const u8,

    const Self = @This();

    fn init_from_file(allocator: Allocator, scratch: Allocator, path: []const u8) !Self {
        const exe_dir = try std.fs.selfExeDirPathAlloc(scratch);
        const full_path = try std.fs.path.join(scratch, &[_][]const u8{ exe_dir, path });

        const file = try std.fs.openFileAbsolute(full_path, .{});
        defer file.close();

        const stat = try file.stat();
        const contents = try file.readToEndAlloc(allocator, stat.size);

        return .{
            .alloc = allocator,
            .buffer = contents,
        };
    }

    fn deinit(self: Self) void {
        self.alloc.free(self.buffer);
    }

    fn whitespace(self: Self) !ArrayList(usize) {
        var found = ArrayList(usize).init(self.alloc);

        for (self.buffer, 0..) |c, i| {
            if (std.ascii.isWhitespace(c)) {
                try found.append(i);
            }
        }

        return found;
    }

    // Eh.. Not really specific enough
    fn tokens(self: Self, separators: []const u8) !ArrayList([]const u8) {
        var found = ArrayList([]const u8).init(self.alloc);

        var start: usize = 0; // Start of the current token
        var in_token = false;

        for (self.buffer, 0..) |character, index| {
            if (isSeparator(character, separators)) {
                if (!in_token) continue;

                // Add the completed token and the separator
                try found.append(self.buffer[start..index]);
                try found.append(self.buffer[index .. index + 1]);
                in_token = false;
            } else if (std.ascii.isWhitespace(character)) {
                if (!in_token) continue;

                // Add the completed token
                try found.append(self.buffer[start..index]);
                in_token = false;
            } else {
                if (in_token) continue;

                // Begin a new token
                start = index;
                in_token = true;
            }
        }
        if (in_token) {
            try found.append(self.buffer[start..]);
        }

        return found;
    }

    fn isSeparator(character: u8, separators: []const u8) bool {
        for (separators) |sep| {
            if (character == sep) return true;
        }
        return false;
    }

    // Make more specific functions that extract key features from the buffer
    // instead of trying to do it all in one function
    fn keywords() ArrayList([]const u8) {}
    fn strings() ArrayList([]const u8) {}
    fn numbers() ArrayList([]const u8) {}
    fn comments() ArrayList([]const u8) {}
};

test "Parse whitespace" {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer {
        const deinit_status = gpa.deinit();
        if (deinit_status == .leak) std.testing.expect(false) catch @panic("TEST FAIL");
    }
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const scratch = arena.allocator();

    const lexer = try Lexer.init_from_file(allocator, scratch, "../../../src/parse.zig");
    defer lexer.deinit();

    const whitespace = try lexer.whitespace();
    defer whitespace.deinit();

    const tokens = try lexer.tokens("(){}[],.");
    defer tokens.deinit();

    for (tokens.items) |token| {
        std.debug.print("{s}\n", .{token});
    }
}
