#include "game.h"
#include "assert.h"

#include <stddef.h>
#include <stdint.h>

Scene *sceneInit() { return NULL; }
void sceneDeinit() {}

void gameTick(void *gameState, uint64_t deltaTime) {
  Scene *scene = (Scene *)gameState;
  assert(scene != NULL);
}

void gameDraw(void *gameState) {
  Scene *scene = (Scene *)gameState;
  assert(scene != NULL);
}
