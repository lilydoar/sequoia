const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn ResourceLib(comptime T: type) type {
    return struct {
        resources: std.ArrayList(T),
        ids: std.StringHashMap(usize),

        const Self = @This();
        pub const ResourceID = usize;

        pub fn init(alloc: std.mem.Allocator) Self {
            return .{
                .resources = std.ArrayList(T).init(alloc),
                .ids = std.StringHashMap(ResourceID).init(alloc),
            };
        }

        pub fn deinit(self: *Self) void {
            self.resources.deinit();
            self.ids.deinit();
        }

        pub fn register(self: *Self, name: []const u8, resource: T) !ResourceID {
            if (self.ids.get(name)) |id| {
                return id;
            }

            try self.ids.put(name, self.resources.items.len);
            errdefer _ = self.ids.remove(name);

            try self.resources.append(resource);
            return self.ids.get(name).?;
        }

        pub fn get(self: Self, id: ResourceID) T {
            std.debug.assert(id < self.resources.items.len);
            return self.resources.items[id];
        }

        pub fn getID(self: Self, name: []const u8) ?ResourceID {
            return self.ids.get(name);
        }
    };
}
