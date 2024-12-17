#include "SDL3/SDL_rect.h"
#include "cglm/types.h"
#include <stddef.h>

typedef struct Context {
} Context;

typedef struct TextureAtlas {
} TextureAtlas;
struct TextureAtlasEntry {
  TextureAtlas *atlas;
  SDL_FRect entry;
};
struct Animation {
  TextureAtlas *atlas;
  size_t frameCount;
  struct TextureAtlasEntry *frames;
  float frameTime;
};

typedef struct Sprite {
  struct TextureAtlasEntry texture;
  vec2 position;
} Sprite;
typedef struct AnimatedSprite {
  struct Animation animation;
  vec2 position;
} AnimatedSprite;

void SEQ_BeginRenderPass(Context *context, TextureAtlas *atlas) {};
void SEQ_EndRenderPass(Context *context) {};

void SEQ_RenderPassDrawSprite(Context *context, Sprite sprite) {};
void SEQ_RenderPassDrawAnimatedSprite(Context *context, AnimatedSprite sprite) {
};
