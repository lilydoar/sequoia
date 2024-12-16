#include "core.h"
#include "game.h"

int main(int argc, const char **argv) {
  Context context;
  if (!initWindow(&context, "", 800, 600, 0))
    return 1;
  if (!initRender(&context))
    return 1;
  return executeLoop(&gameTick, &gameDraw);
}
