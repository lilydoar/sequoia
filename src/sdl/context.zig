const std = @import("std");

const Upload = @import("gpu_upload.zig");
const manager = @import("resource_management.zig");
const shader = @import("shader.zig");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

window: *sdl.SDL_Window,
device: *sdl.SDL_GPUDevice,
gpu_upload_staging: Upload,
shaders: manager.ResourceLib(*sdl.SDL_GPUShader),
pipelines: manager.ResourceLib(*sdl.SDL_GPUGraphicsPipeline),

const Self = @This();

pub fn init(alloc: std.mem.Allocator, app: App, window: Window) !Self {
    try set_app_metadata(app);

    if (!sdl.SDL_InitSubSystem(sdl.SDL_INIT_VIDEO)) {
        return error.SDL_Init_Video;
    }

    const sdl_window = sdl.SDL_CreateWindow(
        app.name.ptr,
        @intCast(window.width),
        @intCast(window.height),
        window.flags,
    ) orelse return error.SDL_CreateWindow;

    const device = sdl.SDL_CreateGPUDevice(
        sdl.SDL_GPU_SHADERFORMAT_DXIL |
            sdl.SDL_GPU_SHADERFORMAT_MSL |
            sdl.SDL_GPU_SHADERFORMAT_SPIRV,
        true,
        null,
    ) orelse return error.SDL_CreateGPUDevice;

    if (!sdl.SDL_ClaimWindowForGPUDevice(device, sdl_window)) {
        return error.SDL_ClaimWindowForGPUDevice;
    }

    var shaders = manager.ResourceLib(*sdl.SDL_GPUShader).init(alloc);
    var pipelines = manager.ResourceLib(*sdl.SDL_GPUGraphicsPipeline)
        .init(alloc);

    // TODO
    // I think I want to move the init of specific shaders to a render context.
    // The shaders and pipelines will still be registered through the sdl context.
    {
        const tri_vert = try shaders.register(
            "tri_vert",
            try shader.fromFile(
                device,
                "assets/gen/shaders/metal/triangle.metal",
                .{
                    .entrypoint = "vertexMain",
                    .format = sdl.SDL_GPU_SHADERFORMAT_MSL,
                    .stage = sdl.SDL_GPU_SHADERSTAGE_VERTEX,
                    .num_storage_buffers = 1,
                },
            ),
        );
        const tri_frag = try shaders.register(
            "tri_frag",
            try shader.fromFile(
                device,
                "assets/gen/shaders/metal/triangle.metal",
                .{
                    .entrypoint = "fragmentMain",
                    .format = sdl.SDL_GPU_SHADERFORMAT_MSL,
                    .stage = sdl.SDL_GPU_SHADERSTAGE_FRAGMENT,
                },
            ),
        );

        // Not certain where this information's init belongs.
        // I think it is strongly linked to the vertex shader.
        // But it may be slightly more generic than that
        const buffer_desc = [_]sdl.SDL_GPUVertexBufferDescription{
            .{
                .slot = 0,
                .pitch = @sizeOf(f32) * 6,
                .input_rate = sdl.SDL_GPU_VERTEXINPUTRATE_VERTEX,
            },
        };
        const vert_attribs = [_]sdl.SDL_GPUVertexAttribute{
            .{
                .location = 0,
                .buffer_slot = 0,
                .format = sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = 0,
            },
            .{
                .location = 1,
                .buffer_slot = 0,
                .format = sdl.SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = @sizeOf(f32) * 2,
            },
        };

        _ = try pipelines.register(
            "triangle",
            sdl.SDL_CreateGPUGraphicsPipeline(
                device,
                &.{
                    .vertex_shader = shaders.get(tri_vert),
                    .fragment_shader = shaders.get(tri_frag),
                    .primitive_type = sdl.SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                    .vertex_input_state = .{
                        .vertex_buffer_descriptions = &buffer_desc,
                        .num_vertex_buffers = buffer_desc.len,
                        .vertex_attributes = &vert_attribs,
                        .num_vertex_attributes = vert_attribs.len,
                    },
                    .rasterizer_state = .{},
                    .multisample_state = .{},
                    .depth_stencil_state = .{},
                    .target_info = .{},
                },
            ) orelse return error.SDL_CreateGPUGraphicsPipeline,
        );
    }

    return .{
        .window = sdl_window,
        .device = device,
        .gpu_upload_staging = Upload.init(alloc),
        .shaders = shaders,
        .pipelines = pipelines,
    };
}

pub fn deinit(self: *Self) void {
    self.pipelines.deinit();
    self.shaders.deinit();
    self.gpu_upload_staging.deinit();
    sdl.SDL_DestroyGPUDevice(self.device);
    sdl.SDL_DestroyWindow(self.window);
    sdl.SDL_Quit();
}

pub const App = struct {
    name: []const u8,
    version: ?[]const u8 = null,
    identifier: ?[]const u8 = null,
    creator: ?[]const u8 = null,
    copyright: ?[]const u8 = null,
    url: ?[]const u8 = null,
};

pub const Window = struct {
    width: usize,
    height: usize,
    flags: sdl.SDL_WindowFlags = 0,
};

fn set_app_metadata(app: App) !void {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_NAME_STRING,
        app.name.ptr,
    )) {
        return error.SDL_SetAppMetadata_Name;
    }
    if (app.version) |version| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_VERSION_STRING,
            version.ptr,
        )) {
            return error.SDL_SetAppMetadata_Version;
        }
    }
    if (app.identifier) |identifier| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_IDENTIFIER_STRING,
            identifier.ptr,
        )) {
            return error.SDL_SetAppMetadata_Identifier;
        }
    }
    if (app.creator) |creator| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_CREATOR_STRING,
            creator.ptr,
        )) {
            return error.SDL_SetAppMetadata_Creator;
        }
    }
    if (app.copyright) |copyright| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_COPYRIGHT_STRING,
            copyright.ptr,
        )) {
            return error.SDL_SetAppMetadata_Copyright;
        }
    }
    if (app.url) |url| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_URL_STRING,
            url.ptr,
        )) {
            return error.SDL_SetAppMetadata_Url;
        }
    }
}
