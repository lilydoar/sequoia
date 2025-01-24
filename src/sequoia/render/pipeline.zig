const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Shader = @import("shader.zig");
const VertexBuffer = @import("vertex_buffer.zig");
const IndexBuffer = @import("index_buffer.zig");

pub const Descriptor = struct {
    primitive: sdl.SDL_GPUPrimitiveType =
        sdl.SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    rasterizer: sdl.SDL_GPURasterizerState = .{},
    multisample: sdl.SDL_GPUMultisampleState = .{},
    depth_stencil: sdl.SDL_GPUDepthStencilState = .{},
    target: sdl.SDL_GPUGraphicsPipelineTargetInfo = .{},
};

const BufferDesc = sdl.SDL_GPUVertexBufferDescription;

const Self = @This();

desc: Descriptor,
vert_shader: Shader,
frag_shader: Shader,
vert_bufs: std.ArrayList(VertexBuffer),
idx_buf: IndexBuffer,
ptr: ?*sdl.SDL_GPUGraphicsPipeline = null,

pub fn init(
    static_alloc: std.mem.Allocator,
    scope_alloc: std.mem.Allocator,
    device: *sdl.SDL_GPUDevice,
    desc: Descriptor,
    vert_shader: Shader,
    frag_shader: Shader,
    vert_bufs: []const VertexBuffer,
    index_buf: IndexBuffer,
) !Self {
    var bufs = std.ArrayList(VertexBuffer).init(static_alloc);
    try bufs.appendSlice(vert_bufs);

    return Self{
        .desc = desc,
        .vert_shader = vert_shader,
        .frag_shader = frag_shader,
        .vert_bufs = bufs,
        .idx_buf = index_buf,
        .ptr = try buildPipeline(
            scope_alloc,
            device,
            desc,
            vert_shader,
            frag_shader,
            vert_bufs,
        ),
    };
}

pub fn deinit(self: Self, device: *sdl.SDL_GPUDevice) void {
    sdl.SDL_ReleaseGPUGraphicsPipeline(device, self.ptr);
    self.vert_shader.deinit(device);
    self.frag_shader.deinit(device);
    for (self.vert_bufs.items) |buf| buf.deinit(device);
    self.vert_bufs.deinit();
    self.idx_buf.deinit(device);
}

pub fn bind(self: Self, pass: *sdl.SDL_GPURenderPass) void {
    sdl.SDL_BindGPUGraphicsPipeline(pass, self.ptr);
    for (self.vert_bufs.items, 0..) |buf, i| buf.bind(pass, @intCast(i));
    self.idx_buf.bind(pass);
}

pub fn draw(self: Self, pass: *sdl.SDL_GPURenderPass) void {
    sdl.SDL_DrawGPUIndexedPrimitives(pass, self.idx_buf.count(), 1, 0, 0, 0);
}

fn buildPipeline(
    scope_alloc: std.mem.Allocator,
    device: *sdl.SDL_GPUDevice,
    desc: Descriptor,
    vert_shader: Shader,
    frag_shader: Shader,
    vert_bufs: []const VertexBuffer,
) !*sdl.SDL_GPUGraphicsPipeline {
    var descs = std.ArrayList(BufferDesc).init(scope_alloc);
    defer descs.deinit();

    var attribs = std.ArrayList(sdl.SDL_GPUVertexAttribute).init(scope_alloc);
    defer attribs.deinit();

    for (vert_bufs, 0..) |buf, slot| {
        try descs.append(.{
            .slot = @intCast(slot),
            .pitch = buf.desc.pitch(),
            .input_rate = sdl.SDL_GPU_VERTEXINPUTRATE_VERTEX,
        });

        var offset: u32 = 0;
        for (buf.desc.attribs.items, 0..) |attr, loc| {
            try attribs.append(.{
                .location = @intCast(loc),
                .buffer_slot = @intCast(slot),
                .format = attr.format,
                .offset = offset,
            });
            offset += attr.size;
        }
    }

    return sdl.SDL_CreateGPUGraphicsPipeline(device, &.{
        .vertex_shader = vert_shader.ptr,
        .fragment_shader = frag_shader.ptr,
        .vertex_input_state = .{
            .vertex_buffer_descriptions = descs.items.ptr,
            .num_vertex_buffers = @intCast(descs.items.len),
            .vertex_attributes = attribs.items.ptr,
            .num_vertex_attributes = @intCast(attribs.items.len),
        },
        .primitive_type = desc.primitive,
        .rasterizer_state = desc.rasterizer,
        .multisample_state = desc.multisample,
        .depth_stencil_state = desc.depth_stencil,
        .target_info = desc.target,
    }) orelse return error.CreateGPUGraphicsPipeline;
}
