const std = @import("std");
const testing = std.testing;

const Allocator = std.mem.Allocator;

// TODO: Zig import of SDL3
// #include "SDL3/SDL_gpu.h"

export fn add(a: i32, b: i32) i32 {
    return a + b;
}

const SDL_Wannabe = struct {
    const CommandBuffer = struct {
        const Queue = struct {};
        const Callable = fn (ctx: anytype) anyerror!void;
    };

    const GPUBuf = struct {};
};
const SDL = SDL_Wannabe;

// Want a zig context that works like golang
const Context = struct {};

const DrawPipeline = struct {
    stages: std.ArrayList(Stage),
    director: Director,

    fn init(alloc: Allocator) !DrawPipeline {
        const new: DrawPipeline = .{
            .stages = std.ArrayList(Stage).init(alloc),
            .director = {},
        };
        return new;
    }
    fn deinit(self: *DrawPipeline) void {
        self.stages.deinit();
    }

    fn runAll(pipeline: DrawPipeline) !void {
        while (try pipeline.director.next() != null) {}
    }

    const Stage = struct {
        const Info = struct {
            const Primitive = enum { Triangles };
            primitive: Primitive,
        };
        const Program = struct {
            const Step = enum { Compute, Vertex, Fragment };

            const Compiled = struct {
                contents: ?[]const u8 = null,
                entry_point: ?[:0]const u8 = null,
            };

            const Slot = struct {
                const Kind = enum {
                    vertex_attrib,
                    vertex_out,
                    fragment_in,
                    fragment_out,
                    uniform,
                    read_storage,
                    write_storage,
                    texture,
                };

                const Format = enum {
                    f32,
                    i32,
                    vec2,
                    vec3,
                    vec4,
                    tex2d,
                    mat4x4,
                };

                kind: Kind,
                format: Format,
                binding: u32 = 0,
            };

            step: Step,
            code: Compiled,
            slots: std.ArrayList(Slot),

            fn init(alloc: Allocator, step: Step) Program {
                return .{
                    .step = step,
                    .code = .{},
                    .slots = std.ArrayList(Slot).init(alloc),
                };
            }
            fn deinit(self: *Program) void {
                self.slots.deinit();
            }

            fn WithSlot(self: Program, slot: Slot) !Program {
                var new_program: Program = self;
                try new_program.slots.append(slot);
                return new_program;
            }
        };

        const Resource = struct {
            const Type = enum { VertexBuf, IndexBuf, TextureView };
        };

        info: ?Info = null,
        compute: ?Program = null,
        vertex: ?Program = null,
        fragment: ?Program = null,

        resources: std.ArrayList(Resource),
        unbound_slots: std.ArrayList(Program.Slot),
        bindings: std.AutoHashMap(Program.Slot, Resource),

        fn init(alloc: Allocator) Stage {
            const new: Stage = .{
                .resources = std.ArrayList(Resource).init(alloc),
                .unbound_slots = std.ArrayList(Program.Slot).init(alloc),
                .bindings = std.AutoHashMap(Program.Slot, Resource).init(alloc),
            };
            return new;
        }
        fn deinit(self: *Stage) void {
            self.resources.deinit();
            self.unbound_slots.deinit();
            self.bindings.deinit();
        }

        const StageCheckError = error{
            RequireAtLeastOneDrawProgram,
            RequireAllSlotsBound,
        };
        fn Check(stage: Stage) !void {
            if (stage.compute == null and
                stage.vertex == null and
                stage.fragment == null)
            {
                return StageCheckError.RequireAtLeastOneDrawProgram;
            }

            if (stage.unbound_slots.items.len > 0) {
                return StageCheckError.RequireAllSlotsBound;
            }
        }

        fn Run(stage: Stage) !void {
            try Check(stage);
        }

        fn WithInfo(stage: Stage, info: Info) Stage {
            var new_stage = stage;
            new_stage.info = info;
            return new_stage;
        }
        fn WithProgram(stage: Stage, program: Program) !Stage {
            var new_stage = stage;
            switch (program.step) {
                .Compute => {
                    new_stage.compute = program;
                },
                .Vertex => {
                    new_stage.vertex = program;
                },
                .Fragment => {
                    new_stage.fragment = program;
                },
            }

            for (program.slots.items) |slot| {
                try new_stage.unbound_slots.append(slot);
            }

            return new_stage;
        }

        fn WithResource(stage: Stage, resource: Resource) Stage {
            var new_stage = stage;
            new_stage.resources.append(resource);
            return new_stage;
        }

        fn BuildCmdBuf(stage: Stage) SDL.CommandBuffer.Callable {
            _ = stage; // autofix
        }
    };

    fn AppendStage(pipeline: DrawPipeline, stage: Stage) !DrawPipeline {
        var new_pipeline = pipeline;
        new_pipeline.stages.append(stage);
        return new_pipeline;
    }

    const Director = struct {
        current: usize = 0,

        fn next() !?Stage {
            return null;
        }
    };
};

test "A stage can't run with zero programs" {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer {
        const deinit_status = gpa.deinit();
        if (deinit_status == .leak) std.testing.expect(false) catch @panic("TEST FAIL");
    }

    var stage = DrawPipeline.Stage.init(allocator);
    defer stage.deinit();

    try testing.expectError(
        DrawPipeline.Stage.StageCheckError.RequireAtLeastOneDrawProgram,
        stage.Run(),
    );
}

test "A stage can't run with unbound slots" {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer {
        const deinit_status = gpa.deinit();
        if (deinit_status == .leak) std.testing.expect(false) catch @panic("TEST FAIL");
    }

    var program = DrawPipeline.Stage.Program.init(allocator, .Vertex);
    defer program.deinit();

    program = try program.WithSlot(.{ .kind = .vertex_attrib, .format = .vec2, .binding = 0 });
    program = try program.WithSlot(.{ .kind = .vertex_attrib, .format = .vec2, .binding = 1 });
    program = try program.WithSlot(.{ .kind = .uniform, .format = .mat4x4 });
    program = try program.WithSlot(.{ .kind = .texture, .format = .tex2d });
    program = try program.WithSlot(.{ .kind = .vertex_out, .format = .vec2 });
    program = try program.WithSlot(.{ .kind = .fragment_in, .format = .vec2 });
    program = try program.WithSlot(.{ .kind = .fragment_out, .format = .vec4 });

    var stage = DrawPipeline.Stage.init(allocator);
    defer stage.deinit();

    stage = try stage.WithProgram(program);
    // try stage.vertex.?.slots.append(.{ .kind = .fragment_out, .format = .vec4 });

    try testing.expectError(
        DrawPipeline.Stage.StageCheckError.RequireAllSlotsBound,
        stage.Run(),
    );
}
