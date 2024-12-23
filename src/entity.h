#include "physics.h"

// TODO
//

typedef enum {
  SHEEP_STATE_IDLE,
  SHEEP_STATE_FOLLOW_POINT,
  SHEEP_STATE_FOLLOW_PATH,
  SHEEP_STATE_GRAZE,
} SheepStateType;

typedef struct {
  SheepStateType state;
  Kinematic kinematic;
  Collider collider;
  
} EntitySheep ;

/*typedef struct {} Component_Sprite;*/
/*typedef struct {} Component_SpriteAnimation;*/
/*typedef struct {} Component_SpriteMarkov;*/


