#ifndef TEXTURE_H
#define TEXTURE_H

#include "SDL3/SDL_gpu.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
} AABB;

#define ANIMATION_NAME_MAX_CHARS 32
#define ANIMATION_FRAMES_MAX_LEN 16
#define ANIMATION_CLIPS_MAX_NUM 64

typedef struct {
  // Texture data
  uint32_t w;
  uint32_t h;
  uint32_t channels;
  unsigned char *pixels;
  size_t pixelsSize;
  // The necessary information for a texture to be used in draw step (I think)
  SDL_GPUTexture *texture;
  SDL_GPUSampler *sampler;
} Texture;

typedef AABB TextureBounds;
typedef struct {
  const char *name[ANIMATION_NAME_MAX_CHARS];
  TextureBounds pixelRect;
  uint32_t durationTicks;
} SpriteFrame;

struct SpriteClip {
  const char *name[ANIMATION_NAME_MAX_CHARS];
  SpriteFrame *frames[ANIMATION_FRAMES_MAX_LEN];
  size_t frameCount;
  // Used for binding texture during draw calls
  Texture *gpuTexture;
  // Used for conversion to normalized UV coordinates
  TextureBounds bounds;
};

struct SpriteClipLib {
  const char *name[ANIMATION_NAME_MAX_CHARS];
  struct SpriteClip *clips[ANIMATION_CLIPS_MAX_NUM];
};

#endif
