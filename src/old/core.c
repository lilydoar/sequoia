#include "core.h"

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_video.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

bool initApp(const char *name, const char *version, const char *identifier) {
  return SDL_SetAppMetadata(name, version, identifier);
}

bool initWindow(struct Context *context, const char *title, uint16_t w,
                uint16_t h, SDL_WindowFlags flags) {
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Initialize SDL Video: %s\n",
                 SDL_GetError());
    return false;
  }

  SDL_Window *sdlWindow = SDL_CreateWindow(title, w, h, flags);
  if (sdlWindow == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create SDL window: %s\n",
                 SDL_GetError());
    return false;
  }

  context->window = malloc(sizeof(struct Window));
  context->window->title = title;
  context->window->width = w;
  context->window->height = h;
  context->window->flags = flags;
  context->window->window = sdlWindow;

  return true;
}

void deinitWindow(struct Context *context) {
  SDL_ReleaseWindowFromGPUDevice(context->renderer->device,
                                 context->window->window);
  SDL_DestroyWindow(context->window->window);
}

bool initRender(struct Context *context) {
  assert(SDL_GPUSupportsShaderFormats(SUPPORTED_SHADER_FORMATS, NULL));
  SDL_GPUDevice *device =
      SDL_CreateGPUDevice(SUPPORTED_SHADER_FORMATS, true, NULL);
  if (device == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create GPU device: %s\n",
                 SDL_GetError());
    return false;
  }

  assert(context->window->window != NULL);
  if (!SDL_ClaimWindowForGPUDevice(device, context->window->window)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Bind GPU device to window: %s\n",
                 SDL_GetError());
    return false;
  }

  context->renderer = malloc(sizeof(struct Render));
  context->renderer->device = device;

  return true;
}

void deinitRender(struct Context *context) {
  SDL_DestroyGPUDevice(context->renderer->device);
}

bool initGame(Context *context, void *state,
              void (*tick)(void *gameState, uint64_t deltaTime),
              void (*draw)(void *gameState)) {
  if (state != NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "game state is NULL\n");
    return false;
  }
  if (tick != NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "game tick is NULL\n");
    return false;
  }
  if (draw != NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "game draw is NULL\n");
    return false;
  }

  context->Game->state = state;
  context->Game->tick = tick;
  context->Game->draw = draw;

  return true;
}
