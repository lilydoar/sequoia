const std = @import("std");
const Build = std.Build;
const OptimizeMode = std.builtin.OptimizeMode;

pub fn build(b: *Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const dep_sokol = b.dependency("sokol", .{
        .target = target,
        .optimize = optimize,
    });
    const hello = b.addExecutable(.{
        .name = "hello",
        .target = target,
        .optimize = optimize,
        .root_source_file = b.path("src/main.zig"),
    });
    hello.root_module.addImport("sokol", dep_sokol.module("sokol"));
    b.installArtifact(hello);
    const run = b.addRunArtifact(hello);
    b.step("run", "Run hello").dependOn(&run.step);
}
