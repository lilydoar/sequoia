#ifndef COMPONENT_H
#define COMPONENT_H

#include "atlas.h"
#include "markov.h"
#include "physics.h"

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

typedef struct {
  SpriteDynamic *sprites;
  size_t statesCount;
} SpriteMarkovMap;

struct SpriteAnimation {
  struct AnimationClip *animation;
  size_t currentFrame;
  uint32_t frameTimeAccumulator;
  enum PlaybackMode mode;
  bool finished;
};
struct AtlasRect SpriteAnimationCurrentRect(struct SpriteAnimation self) {
  return self.animation->frames[self.currentFrame].rect;
}
void SpriteAnimationStep(struct SpriteAnimation *self) {
  if (self->finished) {
    return;
  }

  self->frameTimeAccumulator += 1;

  struct AnimationFrame currentFrame =
      self->animation->frames[self->currentFrame];
  if (self->frameTimeAccumulator < currentFrame.durationTicks) {
    return;
  }

  self->frameTimeAccumulator = 0;
  self->currentFrame++;
  if (self->currentFrame < self->animation->frameCount) {
    return;
  }

  switch (self->mode) {
  case PLAYBACK_LOOP:
    self->currentFrame = 0;
    break;

  case PLAYBACK_ONCE:
    // Stay on the last frame
    self->currentFrame = self->animation->frameCount - 1;
    self->finished = true;
    break;
  }
}

#endif
