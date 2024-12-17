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

void gameTick(void *gameState, uint64_t deltaTime);
void gameDraw(void *gameState);
