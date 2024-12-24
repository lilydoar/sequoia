#include "atlas.h"
#include "component.h"
#include "markov.h"
#include "physics.h"
#include "sequoia_math.h"
#include <stddef.h>
#include <stdint.h>

#include "gen/atlas/resources.atlas.h"

//
// Sheep

enum SheepStateID {
  SHEEP_STATE_ID_IDLE,
  SHEEP_STATE_ID_FOLLOW_POINT,
  SHEEP_STATE_ID_FOLLOW_PATH,
};
DECLARE_STATE_TYPE(SHEEP_IDLE, uint64_t age;)
DECLARE_STATE_TYPE(SHEEP_FOLLOW_POINT, vec2s p;)
DECLARE_STATE_TYPE(SHEEP_FOLLOW_PATH, BezierPath path;)

typedef struct {
  Kinematic kinematic;
  Collider collider;
  MarkovModel *model;
  SpriteMap animations;
  SpritePlayback playback;
} Sheep;

Sheep Sheep_Init(void) {
  Sheep sheep = {0};
  sheep.kinematic = (Kinematic){0};
  sheep.kinematic.mass = (Mass){.mass = 1.0};

  sheep.collider = (Collider){0};
  sheep.collider.circle.radius = 1.0;

  sheep.model = create_model();
  add_state(sheep.model, SHEEP_STATE_ID_IDLE, &(SHEEP_IDLE){0},
            sizeof(uint64_t));
  add_state(sheep.model, SHEEP_STATE_ID_FOLLOW_POINT, &(SHEEP_FOLLOW_POINT){0},
            sizeof(vec2s));
  add_state(sheep.model, SHEEP_STATE_ID_FOLLOW_PATH, &(SHEEP_FOLLOW_PATH){0},
            sizeof(BezierPath));

  set_state(sheep.model, SHEEP_STATE_ID_IDLE);

  sheep.animations = (SpriteMap){0};
  sheep.animations.statesCount = 3;
  sheep.animations.sprites =
      malloc(sizeof(SpriteDynamic) * sheep.animations.statesCount);

  sheep.animations.sprites[0].animation = &ANIM_HAPPY_SHEEP_IDLE;
  sheep.animations.sprites[0].mode = PLAYBACK_FORWARD;

  sheep.animations.sprites[1].animation = &ANIM_HAPPY_SHEEP_BOUNCING;
  sheep.animations.sprites[1].mode = PLAYBACK_FORWARD;

  sheep.animations.sprites[2].animation = &ANIM_HAPPY_SHEEP_BOUNCING;
  sheep.animations.sprites[2].mode = PLAYBACK_FORWARD;

  sheep.playback = (SpritePlayback){0};

  return sheep;
}

void sheep_deinit(Sheep *sheep) { destroy_model(sheep->model); }
