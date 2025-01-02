const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    var deps = std.ArrayList([]const u8).init(b.allocator);
    defer deps.deinit();

    try deps.append("zigimg");

    const lib = b.addStaticLibrary(.{
        .name = "sequoia",
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize,
    });
    add_libraries(b, target, optimize, lib, deps);
    b.installArtifact(lib);

    const exe = b.addExecutable(.{
        .name = "sequoia",
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    add_libraries(b, target, optimize, exe, deps);
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    // Run the app
    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    // Run unit tests
    const lib_unit_tests = b.addTest(.{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize,
    });

    const run_lib_unit_tests = b.addRunArtifact(lib_unit_tests);

    const exe_unit_tests = b.addTest(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);

    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_lib_unit_tests.step);
    test_step.dependOn(&run_exe_unit_tests.step);

    // Compile without running
    const check_exe = b.addExecutable(.{
        .name = "sequoia",
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    check_exe.linkSystemLibrary("SDL3");

    const check_step = b.step("check", "Check compilation");
    check_step.dependOn(&check_exe.step);
}

fn add_libraries(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    step: *std.Build.Step.Compile,
    deps: std.ArrayList([]const u8),
) void {
    step.linkSystemLibrary("SDL3");

    for (deps.items) |name| {
        const mod = b.dependency(name, .{
            .target = target,
            .optimize = optimize,
        });
        step.root_module.addImport(name, mod.module(name));
    }
}
