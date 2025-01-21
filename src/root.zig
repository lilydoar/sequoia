const std = @import("std");

const StateExternal = *anyopaque;
const StateInternal = struct {
    static_alloc: std.heap.GeneralPurposeAllocator(.{}),
    frame_alloc: std.heap.ArenaAllocator,
};

pub export fn init() ?StateExternal {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const static_allocator = gpa.allocator();

    const state = static_allocator.create(StateInternal) catch return null;
    state.static_alloc = gpa;
    state.frame_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);

    return @ptrCast(state);
}

pub export fn deinit(state_opaque: StateExternal) void {
    const state = fromOpaquePtr(state_opaque);
    defer _ = state.static_alloc.deinit();
    defer state.frame_alloc.deinit();
}

pub export fn reload(state_opaque: StateExternal) void {
    const state = fromOpaquePtr(state_opaque);
    _ = state; // autofix
    std.debug.print("reload\n", .{});
}

pub export fn tick(state_opaque: StateExternal) bool {
    const state = fromOpaquePtr(state_opaque);
    _ = state; // autofix
    std.debug.print("tick\n", .{});
    return false;
}

pub export fn draw(state_opaque: StateExternal) void {
    const state = fromOpaquePtr(state_opaque);
    _ = state; // autofix
    std.debug.print("draw\n", .{});
}

fn fromOpaquePtr(ptr: *anyopaque) *StateInternal {
    return @ptrCast(@alignCast(ptr));
}
