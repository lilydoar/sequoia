#ifndef COMPONENT_H
#define COMPONENT_H

#include "atlas.h"

#include <assert.h>
#include <cglm/struct.h>
#include <stdint.h>

typedef struct {
  // FIXME: This shouldn't be named frame
  struct AnimationFrame *sprite;
} SpriteStatic;

enum PlaybackMode {
  PLAYBACK_ONCE,
  PLAYBACK_FORWARD,
  PLAYBACK_REVERSE,
  PLAYBACK_PING_PONG,
};
typedef struct {
  struct AnimationClip *animation;
  enum PlaybackMode mode;
} SpriteDynamic;

typedef struct {
  bool finished;
  uint32_t currentFrame;
  uint32_t frameTimeAccumulator;
} SpritePlayback;
struct AtlasRect SpritePlayback_CurrentRect(SpritePlayback playback,
                                            SpriteDynamic anim) {
  assert(anim.animation->frames);
  assert(playback.currentFrame < anim.animation->frameCount);
  return anim.animation->frames[playback.currentFrame].rect;
}
void SpritePlayback_Step(SpritePlayback *playback, SpriteDynamic anim) {
  if (playback->finished) {
    return;
  }

  playback->frameTimeAccumulator += 1;

  struct AnimationFrame currentFrame =
      anim.animation->frames[playback->currentFrame];
  if (playback->frameTimeAccumulator < currentFrame.durationTicks) {
    return;
  }

  playback->frameTimeAccumulator = 0;
  playback->currentFrame++;
  if (playback->currentFrame < anim.animation->frameCount) {
    return;
  }

  switch (anim.mode) {
  case PLAYBACK_ONCE:
    // Stay on the last frame
    playback->currentFrame = anim.animation->frameCount - 1;
    playback->finished = true;
    break;
  case PLAYBACK_FORWARD:
    playback->currentFrame = 0;
    break;
  default:
    // TODO
    assert(false);
    break;
  }
}

typedef struct {
  SpriteDynamic *sprites;
  size_t statesCount;
} SpriteMap;
SpriteDynamic *SpriteMap_GetSprite(SpriteMap map, size_t id) {
  assert(id < map.statesCount);
  assert(map.sprites);
  return &map.sprites[id];
}

#endif
