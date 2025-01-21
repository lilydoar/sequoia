const std = @import("std");

const State = *anyopaque;
const InitFn = *const fn () ?State;
const DeinitFn = *const fn (State) void;
const ReloadFn = *const fn (State) void;
const TickFn = *const fn (State) bool;
const DrawFn = *const fn (State) void;

const Self = @This();

lib: std.DynLib,
state: State,
fn_init: InitFn,
fn_deinit: DeinitFn,
fn_reload: ReloadFn,
fn_tick: TickFn,
fn_draw: DrawFn,

pub fn init(path: []const u8) !Self {
    var self = Self{
        .lib = try std.DynLib.open(path),
        .state = undefined,
        .fn_init = undefined,
        .fn_deinit = undefined,
        .fn_reload = undefined,
        .fn_draw = undefined,
        .fn_tick = undefined,
    };
    try self.load();
    self.state = self.fn_init() orelse return error.InitFailed;
    return self;
}

fn deinit(self: *Self) void {
    self.fn_deinit(self.state);
    self.lib.close();
}

pub fn reload(self: *Self, path: []const u8) !void {
    self.lib.close();
    self.lib = try std.DynLib.open(path);
    try self.load();
    self.fn_reload(self.state);
}

pub fn tick(self: *Self) bool {
    return self.fn_tick(self.state);
}

pub fn draw(self: *Self) void {
    self.fn_draw(self.state);
}

fn load(self: *Self) !void {
    self.fn_init = self.lib.lookup(@TypeOf(self.fn_init), "init") orelse
        return error.LookupNotFound;
    self.fn_deinit = self.lib.lookup(@TypeOf(self.fn_deinit), "deinit") orelse
        return error.LookupNotFound;
    self.fn_reload = self.lib.lookup(@TypeOf(self.fn_reload), "reload") orelse
        return error.LookupNotFound;
    self.fn_tick = self.lib.lookup(@TypeOf(self.fn_tick), "tick") orelse
        return error.LookupNotFound;
    self.fn_draw = self.lib.lookup(@TypeOf(self.fn_draw), "draw") orelse
        return error.LookupNotFound;
}
