const std = @import("std");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
};

fn main(b: *std.Build, opt: BuildOptions) *std.Build.Step.Compile {
    const exe = b.addExecutable(.{
        .name = "sequoia",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = opt.target,
            .optimize = opt.optimize,
        }),
    });
    return exe;
}

fn sequoiaLib(b: *std.Build, opt: BuildOptions) *std.Build.Step.Compile {
    const lib = b.addSharedLibrary(.{
        .name = "game",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/root.zig"),
            .target = opt.target,
            .optimize = opt.optimize,
        }),
    });
    lib.linkSystemLibrary("SDL3");
    return lib;
}

pub fn addCheck(b: *std.Build, opt: BuildOptions) void {
    const check_lib = sequoiaLib(b, opt);
    const check_step = b.step("check", "Check compilation");
    check_step.dependOn(&check_lib.step);
}

pub fn build(b: *std.Build) void {
    const opt = BuildOptions{
        .target = b.standardTargetOptions(.{}),
        .optimize = b.standardOptimizeOption(.{}),
    };

    const exe = main(b, opt);
    b.installArtifact(exe);

    const lib = sequoiaLib(b, opt);
    b.installArtifact(lib);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    addCheck(b, opt);
}
