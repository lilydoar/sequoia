#ifndef ATLAS_H
#define ATLAS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAX_FRAMES 128
#define MAX_ANIMATION_NAME 64
#define MAX_ANIMATION_FRAMES 128
#define MAX_ANIMATION_CLIPS 16
#define TICKS_PER_SECOND 60

struct AtlasRect {
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
};

struct AnimationFrame {
  const char frameName[MAX_ANIMATION_NAME];
  struct AtlasRect rect;
  uint32_t durationTicks;
};

struct AnimationClip {
  const char name[MAX_ANIMATION_NAME];
  struct AnimationFrame frames[MAX_ANIMATION_FRAMES];
  size_t frameCount;
};

struct AnimationLibrary {
  struct AnimationClip clips[MAX_ANIMATION_CLIPS];
  size_t clipCount;
};

#endif /* ATLAS_H */
