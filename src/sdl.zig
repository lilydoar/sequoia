const std = @import("std");

const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Context = struct {
    window: *sdl.SDL_Window,
    device: *sdl.SDL_GPUDevice,

    pub fn init(app: App, window: Window) !Context {
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

        const sdl_device = sdl.SDL_CreateGPUDevice(
            sdl.SDL_GPU_SHADERFORMAT_DXIL |
                sdl.SDL_GPU_SHADERFORMAT_MSL |
                sdl.SDL_GPU_SHADERFORMAT_SPIRV,
            true,
            null,
        ) orelse return error.SDL_CreateGPUDevice;

        if (!sdl.SDL_ClaimWindowForGPUDevice(sdl_device, sdl_window)) {
            return error.SDL_ClaimWindowForGPUDevice;
        }

        return .{
            .window = sdl_window,
            .device = sdl_device,
        };
    }

    pub fn deinit(self: Context) void {
        sdl.SDL_DestroyGPUDevice(self.device);
        sdl.SDL_DestroyWindow(self.window);
        sdl.SDL_Quit();
    }
};

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

/// Upload data to the GPU
pub const Upload = struct {
    const Item = struct {
        // Data to be uploaded to the GPU
        data: *anyopaque,
        size: usize,
        // Location to upload data to
        location: union(enum) {
            Buffer: sdl.SDL_GPUBuffer,
            Texture: struct {
                width: u32,
                height: u32,
                texture: sdl.SDL_GPUTexture,
            },
        },
    };

    items: std.ArrayList(Item),

    pub fn init(alloc: std.mem.Allocator) Upload {
        return .{
            .items = std.ArrayList(Item).init(alloc),
        };
    }

    pub fn deinit(self: Upload) void {
        self.items.deinit();
    }

    pub fn stage(self: *Upload, item: Item) void {
        _ = self; // autofix
        _ = item; // autofix
    }

    pub fn flush(self: *Upload) void {
        _ = self; // autofix
    }
};
