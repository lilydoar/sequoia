#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_W 800
#define WINDOW_H 600

SDL_GPUShader *loadShader(SDL_GPUDevice *device, const char *file,
                          const char *entryPoint) {
  size_t size;
  void *contents = SDL_LoadFile(file, &size);
  if (contents == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader file load failed: %s\n",
                 SDL_GetError());
    return NULL;
  }

  SDL_GPUShaderCreateInfo info = {
      .code_size = size,
      .code = contents,
      .entrypoint = entryPoint,
      .format = SDL_GPU_SHADERFORMAT_MSL,
      .stage = SDL_GPU_SHADERSTAGE_VERTEX,
      .num_samplers = 0,
      .num_storage_textures = 0,
      .num_storage_buffers = 0,
      .num_uniform_buffers = 0,
  };

  SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
  if (shader == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader creation failed: %s\n",
                 SDL_GetError());
    SDL_free(contents);
    return NULL;
  }

  SDL_free(contents);
  return shader;
}

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

  const char *file = "content/shaders/metal/triangle.metal";
  SDL_GPUShader *vertexShader = loadShader(device, file, "mainVertex");
  if (vertexShader == NULL) {
    return 1;
  }
  SDL_GPUShader *fragmentShader = loadShader(device, file, "mainFragment");
  if (fragmentShader == NULL) {
    return 1;
  }

  SDL_GPUColorTargetDescription description = {
      .format = SDL_GetGPUSwapchainTextureFormat(device, window),
  };
  SDL_GPUGraphicsPipelineCreateInfo info = {
      .vertex_shader = vertexShader,
      .fragment_shader = fragmentShader,
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .target_info =
          {
              .color_target_descriptions = &description,
              .num_color_targets = 1,
          },
  };
  SDL_GPUGraphicsPipeline *pipeline =
      SDL_CreateGPUGraphicsPipeline(device, &info);
  if (pipeline == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Graphics pipeline creation failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_ReleaseGPUShader(device, vertexShader);
  SDL_ReleaseGPUShader(device, fragmentShader);

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
