// Generated code -- DO NOT EDIT
// Generated from assets/gen/atlas/goblins_buildings.json

#include "atlas.h"

static const struct AnimationClip s_wood_tower_blueClip = {
    .name = "Wood_Tower_Blue",
    .frameCount = 5,
    .frames = {
        { .frameName = "Wood_Tower_Blue 0.aseprite", .rect = { 0, 0, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Blue 1.aseprite", .rect = { 256, 0, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Blue 2.aseprite", .rect = { 512, 0, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Blue 3.aseprite", .rect = { 768, 0, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Blue 4.aseprite", .rect = { 1024, 0, 256, 192 }, .durationTicks = 6 },
    }
};

static const struct AnimationClip s_wood_tower_purpleClip = {
    .name = "Wood_Tower_Purple",
    .frameCount = 5,
    .frames = {
        { .frameName = "Wood_Tower_Purple 0.aseprite", .rect = { 512, 192, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Purple 1.aseprite", .rect = { 768, 192, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Purple 2.aseprite", .rect = { 1024, 192, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Purple 3.aseprite", .rect = { 0, 384, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Purple 4.aseprite", .rect = { 256, 384, 256, 192 }, .durationTicks = 6 },
    }
};

static const struct AnimationClip s_wood_tower_redClip = {
    .name = "Wood_Tower_Red",
    .frameCount = 5,
    .frames = {
        { .frameName = "Wood_Tower_Red 0.aseprite", .rect = { 512, 384, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Red 1.aseprite", .rect = { 768, 384, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Red 2.aseprite", .rect = { 1024, 384, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Red 3.aseprite", .rect = { 0, 576, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Red 4.aseprite", .rect = { 256, 576, 256, 192 }, .durationTicks = 6 },
    }
};

static const struct AnimationClip s_wood_tower_yellowClip = {
    .name = "Wood_Tower_Yellow",
    .frameCount = 5,
    .frames = {
        { .frameName = "Wood_Tower_Yellow 0.aseprite", .rect = { 512, 576, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Yellow 1.aseprite", .rect = { 768, 576, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Yellow 2.aseprite", .rect = { 1024, 576, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Yellow 3.aseprite", .rect = { 0, 768, 256, 192 }, .durationTicks = 6 },
        { .frameName = "Wood_Tower_Yellow 4.aseprite", .rect = { 256, 768, 256, 192 }, .durationTicks = 6 },
    }
};

static struct AnimationLibrary g_goblins_buildingsAnimLibrary = {
    .clips = {
        s_wood_tower_blueClip,
        s_wood_tower_purpleClip,
        s_wood_tower_redClip,
        s_wood_tower_yellowClip,
    },
    .clipCount = 4
};
#define ANIM_WOOD_TOWER_BLUE g_goblins_buildingsAnimLibrary.clips[0]
#define ANIM_WOOD_TOWER_PURPLE g_goblins_buildingsAnimLibrary.clips[1]
#define ANIM_WOOD_TOWER_RED g_goblins_buildingsAnimLibrary.clips[2]
#define ANIM_WOOD_TOWER_YELLOW g_goblins_buildingsAnimLibrary.clips[3]
