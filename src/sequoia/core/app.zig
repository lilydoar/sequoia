const sdl = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Self = @This();

name: []const u8,
version: ?[]const u8 = null,
identifier: ?[]const u8 = null,
creator: ?[]const u8 = null,
copyright: ?[]const u8 = null,
url: ?[]const u8 = null,
app_type: ?[]const u8 = null,

pub fn init(name: []const u8) !Self {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_NAME_STRING,
        name.ptr,
    )) return error.SetAppName;
    return Self{
        .name = name,
    };
}

pub fn withVersion(self: *Self, version: []const u8) !void {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_VERSION_STRING,
        version.ptr,
    )) return error.SetAppVersion;
    self.version = version;
}

pub fn withIdentifier(self: *Self, identifier: []const u8) !void {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_IDENTIFIER_STRING,
        identifier.ptr,
    )) return error.SetAppIdentifier;
    self.identifier = identifier;
}

pub fn withCreator(self: *Self, creator: []const u8) !void {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_CREATOR_STRING,
        creator.ptr,
    )) return error.SetAppCreator;
    self.creator = creator;
}

pub fn withCopyright(self: *Self, copyright: []const u8) !void {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_COPYRIGHT_STRING,
        copyright.ptr,
    )) return error.SetAppCopyright;
    self.copyright = copyright;
}

pub fn withUrl(self: *Self, url: []const u8) !void {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_URL_STRING,
        url.ptr,
    )) return error.SetAppUrl;
    self.url = url;
}

pub fn withAppType(self: *Self, app_type: []const u8) !void {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_TYPE_STRING,
        app_type.ptr,
    )) return error.SetAppType;
    self.app_type = app_type;
}
