const std = @import("std");

const Options = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
};

pub fn build(b: *std.Build) !void {
    const opt = Options{
        .target = b.standardTargetOptions(.{}),
        .optimize = b.standardOptimizeOption(.{}),
    };

    const lib = b.addSharedLibrary(.{
        .name = "lib",
        .root_source_file = b.path("src/root.zig"),
        .target = opt.target,
        .optimize = opt.optimize,
    });
    link_system_libs(lib);
    b.installArtifact(lib);

    run_step(b, opt);
    check_step(b, opt);
    try test_step(b, opt);
}

fn link_system_libs(exe: *std.Build.Step.Compile) void {
    exe.linkSystemLibrary("SDL3");
}

fn run_step(b: *std.Build, opt: Options) void {
    const step = b.step("run", "Run the app");

    const exe = b.addExecutable(.{
        .name = "sequoia",
        .root_source_file = b.path("src/main.zig"),
        .target = opt.target,
        .optimize = opt.optimize,
    });
    link_system_libs(exe);
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    step.dependOn(&run_cmd.step);
}

fn check_step(b: *std.Build, opt: Options) void {
    const step = b.step("check", "Compile without running");

    const check = b.addExecutable(.{
        .name = "check",
        .root_source_file = b.path("src/main.zig"),
        .target = opt.target,
        .optimize = opt.optimize,
    });
    link_system_libs(check);

    step.dependOn(&check.step);
}

fn test_step(b: *std.Build, opt: Options) !void {
    const step = b.step("test", "Run tests");

    var dir = try std.fs.cwd().openDir("src", .{ .iterate = true });
    defer dir.close();

    var iter = dir.iterate();
    while (try iter.next()) |file| {
        if (!std.mem.endsWith(u8, file.name, ".zig")) {
            continue;
        }
        const path = b.path(b.pathJoin(&.{ "src", file.name }));
        const file_step = add_tests_from_file(b, opt, path);
        step.dependOn(&file_step.step);
    }
}

fn add_tests_from_file(b: *std.Build, opt: Options, path: std.Build.LazyPath) *std.Build.Step.Run {
    const tests = b.addTest(.{
        .root_source_file = path,
        .target = opt.target,
        .optimize = opt.optimize,
    });
    return b.addRunArtifact(tests);
}
