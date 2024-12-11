#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_video.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_W 800
#define WINDOW_H 600

int main(int argc, char **argv) {
  SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);

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
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Created GPU device: %s",
               SDL_GetGPUDeviceDriver(device));

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

  bool quit = false;
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        quit = true;
        break;
      }
    }

    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == NULL) {
      SDL_Log("Failed to acquire GPU command buffer: %s", SDL_GetError());
      break;
    }

    SDL_GPUTexture *swapchainTexture;
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, window,
                                        &swapchainTexture, NULL, NULL)) {
      SDL_Log("Failed to aquire swap chain texture: %s", SDL_GetError());
      break;
    }
    if (swapchainTexture == NULL) {
      continue;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.texture = swapchainTexture;
    colorTargetInfo.clear_color = (SDL_FColor){0.3f, 0.4f, 0.5f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass *renderPass =
        SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

    SDL_EndGPURenderPass(renderPass);

    SDL_SubmitGPUCommandBuffer(commandBuffer);
  }

  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyWindow(window);
  SDL_DestroyGPUDevice(device);
  SDL_Quit();

  return 0;
}
