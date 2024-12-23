#include "atlas.h"
#include "physics.h"
#include <cglm/struct.h>

enum PlaybackMode {
  PLAYBACK_LOOP,
  PLAYBACK_ONCE,
};
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
