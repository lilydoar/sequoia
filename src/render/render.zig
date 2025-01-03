// A Render struct contains all information necessary to make a render call

const Context = @import("../sdl/context.zig");
const manager = @import("../sdl/resource_management.zig");
const shader = @import("../sdl/shader.zig");

const shaderID = manager.ResourceLib(*sdl.SDL_GPUShader).ResourceID;
const pipelineID = manager.ResourceLib(*sdl.SDL_GPUGraphicsPipeline).ResourceID;

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

vert: shaderID,
frag: shaderID,
pipeline: pipelineID,

const Self = @This();

pub const DrawInfo = union(enum) {
    primitives: struct {
        num_vertices: u32 = 0,
        num_instances: u32 = 0,
        first_vertex: u32 = 0,
        first_instance: u32 = 0,
    },
};
pub fn draw(
    self: Self,
    context: Context,
    cmd_buf: *sdl.SDL_GPUCommandBuffer,
    info: DrawInfo,
) !void {
    const color_targets = [_]sdl.SDL_GPUColorTargetInfo{};
    const pass = sdl.SDL_BeginGPURenderPass(
        cmd_buf,
        &color_targets,
        color_targets.len,
        &.{},
    ) orelse
        return error.SDL_BeginGPURenderPass;
    defer sdl.SDL_EndGPURenderPass(pass);

    sdl.SDL_BindGPUGraphicsPipeline(pass, context.pipelines.get(self.pipeline));

    // TODO: Bind buffers

    switch (info) {
        .primitives => sdl.SDL_DrawGPUPrimitives(
            pass,
            info.primitives.num_vertices,
            info.primitives.num_instances,
            info.primitives.first_vertex,
            info.primitives.first_instance,
        ),
    }
}
