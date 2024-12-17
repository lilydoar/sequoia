#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_stdinc.h"

#include "assert.h"
#include "core.h"
#include "game.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  Context *context = (Context *)SDL_malloc(sizeof(Context));
  if (context == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Allocate application state\n");
    return SDL_APP_FAILURE;
  }

  if (!initApp("game", "0.0.1", "com.example.game"))
    return SDL_APP_FAILURE;
  if (!initWindow(context, "game", 800, 600, 0))
    return SDL_APP_FAILURE;
  if (!initRender(context))
    return SDL_APP_FAILURE;

  Scene scene = {};

  if (!initGame(context, (void *)&scene, &gameTick, &gameDraw))
    return SDL_APP_FAILURE;

  *appstate = context;

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  Context *context = (Context *)appstate;
  assert(context != NULL);

  switch (event->type) {
  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;
  default:
    return SDL_APP_CONTINUE;
  }
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  Context *context = (Context *)appstate;
  assert(context != NULL);

  context->Game->tick(context->Game->state, 1);
  context->Game->draw(context->Game->state);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  Context *context = (Context *)appstate;
  assert(context != NULL);

  deinitRender(context);
  deinitWindow(context);
  SDL_free(context);
}
