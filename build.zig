const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const sdl_dep = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
    });
    const sdl_lib = sdl_dep.artifact("SDL3");

    const lib_mod = b.createModule(.{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize,
    });
    lib_mod.linkLibrary(sdl_lib);

    const lib = b.addLibrary(.{
        .name = "sequoia",
        .root_module = lib_mod,
    });
    b.installArtifact(lib);

    const lib_unit_tests = b.addTest(.{
        .root_module = lib_mod,
    });

    const run_lib_unit_tests = b.addRunArtifact(lib_unit_tests);

    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_lib_unit_tests.step);

    // Examples

    const pong_mod = b.createModule(.{
        .root_source_file = b.path("examples/pong/src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    pong_mod.addImport("sequoia", lib_mod);

    const pong_exe = b.addExecutable(.{
        .name = "pong",
        .root_module = pong_mod,
    });
    b.installArtifact(pong_exe);

    const run_cmd = b.addRunArtifact(pong_exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("examples-pong", "Run the pong example");
    run_step.dependOn(&run_cmd.step);
}
