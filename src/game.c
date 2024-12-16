#include "cglm/types.h"

#include <stddef.h>
#include <stdint.h>

typedef struct Goblin {
  vec2 position;
} Goblin;

typedef struct Knight {
  vec2 position;
} Knight;

typedef struct Scene {
  Goblin *goblins;
  size_t goblinCount;
  Knight *knights;
  size_t knightCount;
} Scene;

Scene *sceneInit() { return NULL; }
void sceneDeinit() {}

/// scene: The running scene
/// deltaTime: The number of milliseconds since the previous frame
void *gameTick(Scene *scene, uint64_t deltaTime) { return NULL; }
void *gameDraw(Scene *scene) { return NULL; }
