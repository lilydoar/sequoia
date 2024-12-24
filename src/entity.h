#include "component.h"
#include "markov.h"
#include "physics.h"
#include "sequoia_math.h"
#include <stddef.h>
#include <stdint.h>

#include "gen/atlas/resources.atlas.c"

//
// Sheep

DECLARE_STATE_TYPE(SHEEP_IDLE, uint64_t age;)
DECLARE_STATE_TYPE(SHEEP_FOLLOW_POINT, vec2s p;)
DECLARE_STATE_TYPE(SHEEP_FOLLOW_PATH, BezierPath path;)

typedef struct {
  Kinematic kinematic;
  Collider collider;
  MarkovModel *model;
  SpriteMarkovMap animations;
} Sheep;

Sheep Sheep_Init(void) {
  Sheep sheep = {0};
  sheep.kinematic = (Kinematic){0};
  sheep.kinematic.mass = (Mass){.mass = 1.0};

  sheep.collider = (Collider){0};
  sheep.collider.circle.radius = 1.0;

  sheep.model = create_model();
  add_state(sheep.model, NULL, sizeof(uint64_t), "idle");
  add_state(sheep.model, NULL, sizeof(vec2s), "follow");
  add_state(sheep.model, NULL, sizeof(BezierPath), "path");
  set_initial_state(sheep.model, 0);

  sheep.animations = (SpriteMarkovMap){0};
  sheep.animations.statesCount = 3;
  sheep.animations.sprites = (SpriteDynamic[]){
      {&ANIM_HAPPY_SHEEP_IDLE, PLAYBACK_FORWARD},
      {&ANIM_HAPPY_SHEEP_BOUNCING, PLAYBACK_FORWARD},
      {&ANIM_HAPPY_SHEEP_BOUNCING, PLAYBACK_FORWARD},
  };
}

void sheep_deinit(Sheep *sheep) { destroy_model(sheep->model); }
