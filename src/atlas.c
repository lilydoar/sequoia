#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FRAMES 128
#define MAX_ANIMATION_NAME 64
#define MAX_ANIMATION_FRAMES 128
#define MAX_ANIMATION_CLIPS 16
#define TICKS_PER_SECOND 60

// Simple rect for storing sprite subregion
struct AtlasRect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

struct AnimationFrame {
    const char frameName[64];
    struct AtlasRect rect;
    uint32_t duration_ticks;
};

typedef enum {
    PLAYBACK_LOOP,
    PLAYBACK_ONCE,
} PlaybackMode;

struct AnimationClip {
    const char name[MAX_ANIMATION_NAME];
    PlaybackMode mode;
    struct AnimationFrame frames[MAX_ANIMATION_FRAMES];
    size_t frame_count;
};

struct AnimationLibrary {
    struct AnimationClip clips[MAX_ANIMATION_CLIPS];
    size_t clip_count;
};

// ------------------------------------------------------------------
// Hardcoded data starts here
// ------------------------------------------------------------------

// All frames from the JSON have duration 100 ms => 6 ticks
static const int ASEPRITE_DURATION_TICKS = 6;

static const struct AnimationClip s_explosionsClip = {
    .name = "Explosions",
    .frame_count = 9,
    .mode = PLAYBACK_ONCE,
    .frames = {
        {
            .frameName = "Explosions 0.aseprite",
            .rect = {0, 0, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Explosions 1.aseprite",
            .rect = {192, 0, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Explosions 2.aseprite",
            .rect = {384, 0, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Explosions 3.aseprite",
            .rect = {576, 0, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Explosions 4.aseprite",
            .rect = {0, 192, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Explosions 5.aseprite",
            .rect = {192, 192, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Explosions 6.aseprite",
            .rect = {384, 192, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Explosions 7.aseprite",
            .rect = {576, 192, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Explosions 8.aseprite",
            .rect = {0, 384, 192, 192},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        }
    }
};

static const struct AnimationClip s_fireClip = {
    .name = "Fire",
    .frame_count = 7,
    .mode = PLAYBACK_LOOP,
    .frames = {
        {
            .frameName = "Fire 0.aseprite",
            .rect = {192, 384, 128, 128},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Fire 1.aseprite",
            .rect = {320, 384, 128, 128},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Fire 2.aseprite",
            .rect = {448, 384, 128, 128},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Fire 3.aseprite",
            .rect = {576, 384, 128, 128},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Fire 4.aseprite",
            .rect = {192, 512, 128, 128},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Fire 5.aseprite",
            .rect = {320, 512, 128, 128},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        },
        {
            .frameName = "Fire 6.aseprite",
            .rect = {448, 512, 128, 128},
            .duration_ticks = ASEPRITE_DURATION_TICKS
        }
    }
};

static struct AnimationLibrary g_effectsAnimLibrary = {
    .clips = {
      s_explosionsClip,
      s_fireClip,
    },
    .clip_count = 2
};

#define ANIM_EXPLOSIONS g_effectsAnimLibrary.clips[0]
#define ANIM_FIRE g_effectsAnimLibrary.clips[1]

// ------------------------------------------------------------------
// Usage Example (in your code):
//   - #include "effects_anim_data.h"  // or whatever name you choose
//   - Access: g_effectsAnimLibrary.clips[0] => "Explosions" clip
//   - Access: g_effectsAnimLibrary.clips[1] => "Fire" clip
// ------------------------------------------------------------------
