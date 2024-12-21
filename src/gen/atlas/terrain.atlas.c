// Generated code -- DO NOT EDIT
// Generated from assets/gen/atlas/terrain.json

#include "atlas.h"

static const struct AnimationClip s_foamClip = {
    .name = "Foam",
    .frameCount = 8,
    .frames = {
        { .frameName = "Foam 0.aseprite", .rect = { 1152, 512, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Foam 1.aseprite", .rect = { 960, 512, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Foam 2.aseprite", .rect = { 768, 512, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Foam 3.aseprite", .rect = { 576, 512, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Foam 4.aseprite", .rect = { 0, 512, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Foam 5.aseprite", .rect = { 384, 448, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Foam 6.aseprite", .rect = { 192, 448, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Foam 7.aseprite", .rect = { 384, 256, 192, 192 }, .durationTicks = 6 },
    }
};

static const struct AnimationClip s_rocksClip = {
    .name = "Rocks",
    .frameCount = 32,
    .frames = {
        { .frameName = "Rocks 0.aseprite", .rect = { 1728, 768, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 1.aseprite", .rect = { 1600, 768, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 2.aseprite", .rect = { 1472, 768, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 3.aseprite", .rect = { 1344, 768, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 4.aseprite", .rect = { 384, 768, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 5.aseprite", .rect = { 256, 768, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 6.aseprite", .rect = { 128, 768, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 7.aseprite", .rect = { 1216, 704, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 8.aseprite", .rect = { 1088, 704, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 9.aseprite", .rect = { 192, 640, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 10.aseprite", .rect = { 1856, 512, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 11.aseprite", .rect = { 1856, 768, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 12.aseprite", .rect = { 1920, 256, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 13.aseprite", .rect = { 1920, 384, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 14.aseprite", .rect = { 1344, 512, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 15.aseprite", .rect = { 1472, 512, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 16.aseprite", .rect = { 1600, 512, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 17.aseprite", .rect = { 1728, 512, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 18.aseprite", .rect = { 1920, 128, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 19.aseprite", .rect = { 1920, 0, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 20.aseprite", .rect = { 320, 640, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 21.aseprite", .rect = { 448, 640, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 22.aseprite", .rect = { 1344, 640, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 23.aseprite", .rect = { 1472, 640, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 24.aseprite", .rect = { 1600, 640, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 25.aseprite", .rect = { 1728, 640, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 26.aseprite", .rect = { 1856, 640, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 27.aseprite", .rect = { 0, 704, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 28.aseprite", .rect = { 576, 704, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 29.aseprite", .rect = { 704, 704, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 30.aseprite", .rect = { 832, 704, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Rocks 31.aseprite", .rect = { 960, 704, 128, 128 }, .durationTicks = 6 },
    }
};

static struct AnimationLibrary g_terrainAnimLibrary = {
    .clips = {
        s_foamClip,
        s_rocksClip,
    },
    .clipCount = 2
};
#define ANIM_FOAM g_terrainAnimLibrary.clips[0]
#define ANIM_ROCKS g_terrainAnimLibrary.clips[1]
