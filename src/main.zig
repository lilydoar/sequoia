const std = @import("std");

const sokol = @import("sokol");
const log = sokol.log;
const graphics = sokol.gfx;
const app = sokol.app;
const glue = sokol.glue;
const shader = @import("shaders/triangle.glsl.zig");

const state = struct {
    var bind: graphics.Bindings = .{};
    var pip: graphics.Pipeline = .{};
};

export fn init() void {
    graphics.setup(.{
        .environment = glue.environment(),
        .logger = .{ .func = log.func },
    });

    // create vertex buffer with triangle vertices
    state.bind.vertex_buffers[0] = graphics.makeBuffer(.{
        .data = graphics.asRange(&[_]f32{
            // positions         colors
            0.0,  0.5,  0.5, 1.0, 0.0, 0.0, 1.0,
            0.5,  -0.5, 0.5, 0.0, 1.0, 0.0, 1.0,
            -0.5, -0.5, 0.5, 0.0, 0.0, 1.0, 1.0,
        }),
    });

    // create a shader and pipeline object
    var vertex_layout = graphics.VertexLayoutState{};
    vertex_layout.attrs[shader.ATTR_triangle_position].format = .FLOAT3;
    vertex_layout.attrs[shader.ATTR_triangle_color0].format = .FLOAT4;

    state.pip = graphics.makePipeline(.{
        .shader = graphics.makeShader(
            shader.triangleShaderDesc(graphics.queryBackend()),
        ),
        .layout = vertex_layout,
    });
}

export fn frame() void {
    graphics.beginPass(.{ .swapchain = glue.swapchain() });
    graphics.applyPipeline(state.pip);
    graphics.applyBindings(state.bind);
    graphics.draw(0, 3, 1);
    graphics.endPass();
    graphics.commit();
}

export fn cleanup() void {
    graphics.shutdown();
}

pub fn main() void {
    app.run(.{
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .width = 640,
        .height = 480,
        .icon = .{ .sokol_default = true },
        .window_title = "triangle.zig",
        .logger = .{ .func = log.func },
    });
}

test "hello world" {
    std.debug.assert(true);
}
