// Generated code -- DO NOT EDIT
// Generated from assets/gen/atlas/effects.json

#include "atlas.h"

static const struct AnimationClip s_explosionsClip = {
    .name = "Explosions",
    .frameCount = 9,
    .frames = {
        { .frameName = "Explosions 0.aseprite", .rect = { 0, 0, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Explosions 1.aseprite", .rect = { 192, 0, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Explosions 2.aseprite", .rect = { 384, 0, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Explosions 3.aseprite", .rect = { 576, 0, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Explosions 4.aseprite", .rect = { 0, 192, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Explosions 5.aseprite", .rect = { 192, 192, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Explosions 6.aseprite", .rect = { 384, 192, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Explosions 7.aseprite", .rect = { 576, 192, 192, 192 }, .durationTicks = 6 },
        { .frameName = "Explosions 8.aseprite", .rect = { 0, 384, 192, 192 }, .durationTicks = 6 },
    }
};

static const struct AnimationClip s_fireClip = {
    .name = "Fire",
    .frameCount = 7,
    .frames = {
        { .frameName = "Fire 0.aseprite", .rect = { 192, 384, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Fire 1.aseprite", .rect = { 320, 384, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Fire 2.aseprite", .rect = { 448, 384, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Fire 3.aseprite", .rect = { 576, 384, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Fire 4.aseprite", .rect = { 192, 512, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Fire 5.aseprite", .rect = { 320, 512, 128, 128 }, .durationTicks = 6 },
        { .frameName = "Fire 6.aseprite", .rect = { 448, 512, 128, 128 }, .durationTicks = 6 },
    }
};

static struct AnimationLibrary g_effectsAnimLibrary = {
    .clips = {
        s_explosionsClip,
        s_fireClip,
    },
    .clipCount = 2
};
#define ANIM_EXPLOSIONS g_effectsAnimLibrary.clips[0]
#define ANIM_FIRE g_effectsAnimLibrary.clips[1]
