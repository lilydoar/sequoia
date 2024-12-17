#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include <stdint.h>

#define SUPPORTED_SHADER_FORMATS (SDL_GPU_SHADERFORMAT_MSL)

typedef struct Context {
  struct Window *window;
  struct Render *renderer;
  struct Game *Game;
} Context;

struct Window {
  const char *title;
  uint16_t width;
  uint16_t height;
  SDL_WindowFlags flags;
  SDL_Window *window;
};

struct Render {
  SDL_GPUDevice *device;
};

struct Game {
  void *state;
  void (*tick)(void *gameState, uint64_t deltaTime);
  void (*draw)(void *gameState);
};

bool initApp(const char *name, const char *version, const char *identifier);

bool initWindow(Context *context, const char *title, uint16_t w, uint16_t h,
                SDL_WindowFlags flags);
void deinitWindow(Context *context);

bool initRender(Context *context);
void deinitRender(Context *context);

bool initTextures();
bool initSounds();

bool initGame(Context *context, void *state,
              void (*tick)(void *gameState, uint64_t deltaTime),
              void (*draw)(void *gameState));
