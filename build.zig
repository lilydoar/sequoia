const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Basic artifacts
    const artifacts = struct {
        generate: *std.Build.Step.Compile,
        main: *std.Build.Step.Compile,
        lib: *std.Build.Step.Compile,
    }{
        .generate = b.addExecutable(.{
            .name = "gen",
            .root_source_file = .{
                .src_path = .{
                    .owner = b,
                    .sub_path = "generate.zig",
                },
            },
            .target = target,
            .optimize = .ReleaseSafe,
        }),
        .main = b.addExecutable(.{
            .name = "sequoia",
            .root_source_file = .{
                .src_path = .{
                    .owner = b,
                    .sub_path = "src/main.zig",
                },
            },
            .target = target,
            .optimize = optimize,
        }),
        .lib = b.addStaticLibrary(.{
            .name = "sequoia",
            .root_source_file = .{
                .src_path = .{
                    .owner = b,
                    .sub_path = "src/root.zig",
                },
            },
            .target = target,
            .optimize = optimize,
        }),
    };

    // Install all artifacts
    inline for (std.meta.fields(@TypeOf(artifacts))) |field| {
        b.installArtifact(@field(artifacts, field.name));
    }

    // Run command setup
    const run_cmd = b.addRunArtifact(artifacts.main);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);

    // Test configurations
    const tests = struct {
        lib: *std.Build.Step,
        exe: *std.Build.Step,
        src: *std.Build.Step,
    }{
        .lib = try addTestsInFile(b, .{
            .name = "test-lib",
            .path = "src/root.zig",
            .target = target,
        }),
        .exe = try addTestsInFile(b, .{
            .name = "test-exe",
            .path = "src/main.zig",
            .target = target,
        }),
        .src = try addTestsInFiles(b, .{
            .name = "test-src",
            .path = "src",
            .target = target,
        }),
    };

    // Command steps
    const steps: Steps = .{
        .run = b.step("run", "Run the app"),
        .testing = b.step("test", "Run all tests"),
        .check = b.step("check", "Run basic tests"),
        .verify = b.step("verify", "Run exhaustive tests"),
        .generate = b.step("gen", "Run all code gen scripts"),
        .binary = b.step("cmd", "Build a runnable binary"),
    };

    // Wire up dependencies
    steps.run.dependOn(&run_cmd.step);
    steps.testing.dependOn(tests.lib);
    steps.testing.dependOn(tests.exe);
    steps.testing.dependOn(tests.src);
    steps.check.dependOn(tests.lib);
    steps.verify.dependOn(steps.testing);
    const gen_run = b.addRunArtifact(artifacts.generate);
    steps.generate.dependOn(&gen_run.step);
    steps.binary.dependOn(&artifacts.main.step);
}

const Steps = struct {
    run: *std.Build.Step,
    testing: *std.Build.Step,
    check: *std.Build.Step,
    verify: *std.Build.Step,
    generate: *std.Build.Step,
    binary: *std.Build.Step,
};

const OptModes = struct {
    const Self = @This();
    modes: []const std.builtin.OptimizeMode,

    fn all() Self {
        return .{ .modes = &.{
            .Debug,
            .ReleaseSafe,
            .ReleaseFast,
            .ReleaseSmall,
        } };
    }

    fn single(mode: std.builtin.OptimizeMode) Self {
        return .{ .modes = &.{mode} };
    }
};

const TestConfig = struct {
    name: []const u8,
    path: []const u8,
    target: std.Build.ResolvedTarget,
    opt_modes: OptModes = OptModes.all(),
};

fn addTestsInFile(b: *std.Build, config: TestConfig) !*std.Build.Step {
    const desc = try std.fmt.allocPrint(b.allocator, "Run tests for {s}", .{config.path});
    const test_step = b.step(config.name, desc);

    for (config.opt_modes.modes) |mode| {
        const test_exe = b.addTest(.{
            .name = std.fs.path.basename(config.path),
            .root_source_file = .{ .src_path = .{
                .owner = b,
                .sub_path = config.path,
            } },
            .target = config.target,
            .optimize = mode,
        });

        const run_cmd = b.addRunArtifact(test_exe);
        test_step.dependOn(&run_cmd.step);
    }

    return test_step;
}

fn addTestsInFiles(b: *std.Build, config: TestConfig) !*std.Build.Step {
    const desc = try std.fmt.allocPrint(b.allocator, "Run tests in {s}", .{config.path});
    const test_step = b.step(config.name, desc);

    var dir = try std.fs.openDirAbsolute(config.path, .{ .iterate = true });
    defer dir.close();

    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    while (try walker.next()) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.path, ".zig")) continue;

        const full_path = try std.fs.path.join(b.allocator, &.{ config.path, entry.path });
        defer b.allocator.free(full_path);

        const file_config = TestConfig{
            .name = try std.fmt.allocPrint(b.allocator, "test-{s}", .{entry.path}),
            .path = full_path,
            .target = config.target,
            .opt_modes = config.opt_modes,
        };

        const file_tests = try addTestsInFile(b, file_config);
        test_step.dependOn(file_tests);
    }

    return test_step;
}
