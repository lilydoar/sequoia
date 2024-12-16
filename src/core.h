#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include <stdint.h>

#define SUPPORTED_SHADER_FORMATS (SDL_GPU_SHADERFORMAT_MSL)

typedef struct Context {
  struct Window *window;
  struct Render *renderer;
  SDL_AudioStream *playback;
  void *game;
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

bool initWindow(Context *context, const char *title, uint16_t w, uint16_t h,
                SDL_WindowFlags flags);
void deinitWindow(Context *context);

bool initRender(Context *context);
void deinitRender(Context *context);

bool initTextures();
bool initSounds();

bool executeLoop(void tick(uint64_t), void draw());
