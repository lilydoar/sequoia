#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_W 800
#define WINDOW_H 600

int main(int argc, char **argv) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL initialization failed: %s\n",
                 SDL_GetError());
    return 1;
  }

  SDL_GPUShaderFormat flags = SDL_GPU_SHADERFORMAT_SPIRV |
                              SDL_GPU_SHADERFORMAT_DXIL |
                              SDL_GPU_SHADERFORMAT_MSL;
  SDL_GPUDevice *device = SDL_CreateGPUDevice(flags, true, NULL);
  if (device == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GPU Device creation failed: %s\n",
                 SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow(NULL, WINDOW_W, WINDOW_H, 0);
  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Window creation failed: %s\n",
                 SDL_GetError());
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    return 1;
  }

  if (!SDL_ClaimWindowForGPUDevice(device, window)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GPU failed to claim window: %s\n",
                 SDL_GetError());
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyWindow(window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    return 1;
  }

  SDL_strstr("", "");

  bool shouldClose = false;
  while (!shouldClose) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        shouldClose = true;
        break;
      }
    }
  }

  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyWindow(window);
  SDL_DestroyGPUDevice(device);
  SDL_Quit();

  return 0;
}
