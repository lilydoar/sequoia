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

pub fn set_metadata(self: Self) !void {
    if (!sdl.SDL_SetAppMetadataProperty(
        sdl.SDL_PROP_APP_METADATA_NAME_STRING,
        self.name.ptr,
    )) {
        return error.SDL_SetAppMetadata_Name;
    }
    if (self.version) |version| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_VERSION_STRING,
            version.ptr,
        )) {
            return error.SDL_SetAppMetadata_Version;
        }
    }
    if (self.identifier) |identifier| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_IDENTIFIER_STRING,
            identifier.ptr,
        )) {
            return error.SDL_SetAppMetadata_Identifier;
        }
    }
    if (self.creator) |creator| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_CREATOR_STRING,
            creator.ptr,
        )) {
            return error.SDL_SetAppMetadata_Creator;
        }
    }
    if (self.copyright) |copyright| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_COPYRIGHT_STRING,
            copyright.ptr,
        )) {
            return error.SDL_SetAppMetadata_Copyright;
        }
    }
    if (self.url) |url| {
        if (!sdl.SDL_SetAppMetadataProperty(
            sdl.SDL_PROP_APP_METADATA_URL_STRING,
            url.ptr,
        )) {
            return error.SDL_SetAppMetadata_Url;
        }
    }
}
