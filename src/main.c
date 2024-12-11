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

struct Vertex {
  float position[3]; // x, y, z position
  float color[3];    // r, g, b color
} Vertex;

SDL_GPUShader *loadShader(SDL_GPUDevice *device, const char *file,
                          const char *entryPoint, SDL_GPUShaderStage stage) {
  size_t size;
  void *contents = SDL_LoadFile(file, &size);
  if (contents == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader load from file failed: %s\n",
                 SDL_GetError());
    return NULL;
  }

  SDL_GPUShaderCreateInfo info = {
      .code_size = size,
      .code = contents,
      .entrypoint = entryPoint,
      .format = SDL_GPU_SHADERFORMAT_MSL,
      .stage = stage,
      .num_samplers = 0,
      .num_storage_textures = 0,
      .num_storage_buffers = 1,
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
  SDL_GPUShader *vertex =
      loadShader(device, file, "vertexMain", SDL_GPU_SHADERSTAGE_VERTEX);
  if (vertex == NULL) {
    return 1;
  }
  SDL_GPUShader *fragment =
      loadShader(device, file, "fragmentMain", SDL_GPU_SHADERSTAGE_FRAGMENT);
  if (fragment == NULL) {
    return 1;
  }

  SDL_GPUVertexBufferDescription vertexBufferDesc = {
      .slot = 0,
      .pitch = sizeof(float) * 6,
      .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
      .instance_step_rate = 0,
  };
  SDL_GPUVertexAttribute vertexAttributes[] = {
      {
          .location = 0,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
          .offset = 0,
      },
      {
          .location = 1,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
          .offset = sizeof(float) * 3,
      },
  };
  SDL_GPUColorTargetDescription colorTarget = {
      .format = SDL_GetGPUSwapchainTextureFormat(device, window),
  };
  SDL_GPUGraphicsPipelineCreateInfo info = {
      .vertex_shader = vertex,
      .fragment_shader = fragment,
      .vertex_input_state =
          {
              .vertex_buffer_descriptions = &vertexBufferDesc,
              .num_vertex_buffers = 1,
              .vertex_attributes = vertexAttributes,
              .num_vertex_attributes = 2,
          },
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .target_info =
          {
              .color_target_descriptions = &colorTarget,
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

  SDL_ReleaseGPUShader(device, vertex);
  SDL_ReleaseGPUShader(device, fragment);

  struct Vertex triangleVertices[3] = {
      {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Bottom left (red)
      {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // Bottom right (green)
      {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}    // Top (blue)
  };

  SDL_GPUTransferBufferCreateInfo vertexTransferBufferInfo = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = sizeof(triangleVertices),
      .props = 0,
  };
  SDL_GPUTransferBuffer *vertexTransferBuffer =
      SDL_CreateGPUTransferBuffer(device, &vertexTransferBufferInfo);

  struct Vertex *vertexTransferData =
      SDL_MapGPUTransferBuffer(device, vertexTransferBuffer, false);
  if (vertexTransferData == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Mapping GPU transfer buffer failed: %s\n", SDL_GetError());
    return 1;
  }

  vertexTransferData[0] = triangleVertices[0];
  vertexTransferData[1] = triangleVertices[1];
  vertexTransferData[2] = triangleVertices[2];

  SDL_UnmapGPUTransferBuffer(device, vertexTransferBuffer);

  SDL_GPUBufferCreateInfo vertexBufferInfo = {
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
      .size = sizeof(triangleVertices),
      .props = 0,
  };
  SDL_GPUBuffer *vertexBuffer = SDL_CreateGPUBuffer(device, &vertexBufferInfo);
  if (vertexBuffer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vertex buffer creation failed: %s\n",
                 SDL_GetError());
    return 1;
  }
  SDL_SetGPUBufferName(device, vertexBuffer, "Vertex Buffer");

  // Identity matrix
  // clang-format off
  float modelViewProjectionMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
  // clang-format on

  SDL_GPUTransferBufferCreateInfo uniformTransferBufferInfo = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = sizeof(modelViewProjectionMatrix),
      .props = 0,
  };
  SDL_GPUTransferBuffer *uniformTransferBuffer =
      SDL_CreateGPUTransferBuffer(device, &uniformTransferBufferInfo);

  float *uniformTransferData =
      SDL_MapGPUTransferBuffer(device, uniformTransferBuffer, false);
  if (uniformTransferData == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Mapping GPU transfer buffer failed: %s\n", SDL_GetError());
    return 1;
  }

  for (int i = 0; i < 16; i++) {
    uniformTransferData[i] = modelViewProjectionMatrix[i];
  }

  SDL_UnmapGPUTransferBuffer(device, uniformTransferBuffer);

  SDL_GPUBufferCreateInfo uniformBufferInfo = {
      .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
      .size = sizeof(modelViewProjectionMatrix),
      .props = 0,
  };
  SDL_GPUBuffer *uniformBuffer =
      SDL_CreateGPUBuffer(device, &uniformBufferInfo);
  if (uniformBuffer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Uniform buffer creation failed: %s\n",
                 SDL_GetError());
    return 1;
  }
  SDL_SetGPUBufferName(device, uniformBuffer, "Uniform Buffer");

  SDL_GPUCommandBuffer *uploadCommandBuffer =
      SDL_AcquireGPUCommandBuffer(device);
  if (uploadCommandBuffer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Failed to acquire GPU command buffer: %s", SDL_GetError());
    return 1;
  }

  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

  SDL_GPUTransferBufferLocation vertexSource = {
      .transfer_buffer = vertexTransferBuffer,
      .offset = 0,
  };
  SDL_GPUBufferRegion vertexDest = {
      .buffer = vertexBuffer,
      .offset = 0,
      .size = sizeof(triangleVertices),
  };
  SDL_UploadToGPUBuffer(copyPass, &vertexSource, &vertexDest, false);

  SDL_GPUTransferBufferLocation uniformSource = {
      .transfer_buffer = uniformTransferBuffer,
      .offset = 0,
  };
  SDL_GPUBufferRegion uniformDest = {
      .buffer = uniformBuffer,
      .offset = 0,
      .size = sizeof(modelViewProjectionMatrix),
  };
  SDL_UploadToGPUBuffer(copyPass, &uniformSource, &uniformDest, false);

  SDL_EndGPUCopyPass(copyPass);
  SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

  SDL_ReleaseGPUTransferBuffer(device, vertexTransferBuffer);
  SDL_ReleaseGPUTransferBuffer(device, uniformTransferBuffer);

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

    SDL_GPUCommandBuffer *renderCommandBuffer =
        SDL_AcquireGPUCommandBuffer(device);
    if (renderCommandBuffer == NULL) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Failed to acquire GPU command buffer: %s", SDL_GetError());
      break;
    }

    SDL_GPUTexture *swapchainTexture;
    if (!SDL_AcquireGPUSwapchainTexture(renderCommandBuffer, window,
                                        &swapchainTexture, NULL, NULL)) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Failed to aquire swap chain texture: %s", SDL_GetError());
      break;
    }
    if (swapchainTexture == NULL) {
      SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Swapchain texture is NULL\n");
      continue;
    }

    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.texture = swapchainTexture;
    colorTargetInfo.clear_color = (SDL_FColor){0.3f, 0.4f, 0.5f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass *renderPass =
        SDL_BeginGPURenderPass(renderCommandBuffer, &colorTargetInfo, 1, NULL);

    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

    SDL_BindGPUVertexStorageBuffers(renderPass, 0, &uniformBuffer, 1);

    SDL_GPUBufferBinding binding = {
        .buffer = vertexBuffer,
        .offset = 0,
    };
    SDL_BindGPUVertexBuffers(renderPass, 0, &binding, 1);

    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(renderCommandBuffer);
  }

  SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
  SDL_ReleaseGPUBuffer(device, vertexBuffer);
  SDL_ReleaseGPUBuffer(device, uniformBuffer);

  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyWindow(window);
  SDL_DestroyGPUDevice(device);
  SDL_Quit();

  return 0;
}
