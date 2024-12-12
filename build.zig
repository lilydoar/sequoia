const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // FIXME: This tries to build the project like a zig program
    // https://zig.news/kristoff/make-zig-your-c-c-build-system-28g5
    const exe = b.addExecutable(.{
        .name = "program",
        .root_source_file = b.path("src/main.c"),
        .target = target,
        .optimize = optimize,
    });

    exe.addIncludePath(.{ .cwd_relative = "/usr/local/include/SDL3" });

    exe.addIncludePath(b.path("external/cglm/include"));
    exe.addIncludePath(b.path("external/stb"));

    exe.addObjectFile(b.path("external/cglm/build/libcglm.a"));

    exe.linkSystemLibrary("SDL3");
    exe.linkLibC();

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
