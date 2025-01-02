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
            errdefer self.ids.remove(name);

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

// const PipelineID = usize;
//
// pipelines: std.ArrayList(*sdl.SDL_GPUGraphicsPipeline),
// ids: std.StringHashMap(PipelineID),
//
// const Self = @This();
//
// pub fn init(alloc: std.mem.Allocator) Self {
//     return .{
//         .pipelines = std.ArrayList(*sdl.SDL_GPUGraphicsPipeline).init(alloc),
//         .ids = std.StringHashMap(PipelineID).init(alloc),
//     };
// }
//
// pub fn deinit(self: Self) void {
//     self.pipelines.deinit();
// }
//
// pub fn register(self: *Self, name: []const u8, pipeline: *sdl.SDL_GPUGraphicsPipeline) !PipelineID {
//     const id = self.ids.get(name);
//     if (id != null) {
//         return id;
//     }
//
//     try self.ids.put(name, self.pipelines.items.len);
//     errdefer self.ids.remove(name);
//
//     try self.pipelines.append(pipeline);
//     return self.ids.get(name);
// }
//
// pub fn get(self: Self, id: PipelineID) *sdl.SDL_GPUGraphicsPipeline {
//     std.debug.assert(id < self.pipelines.items.len);
//     return self.pipelines[id];
// }
//
// pub fn getID(self: Self, name: []const u8) ?PipelineID {
//     return self.ids.get(name);
// }
