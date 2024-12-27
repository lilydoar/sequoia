#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"

#define GPU_TRANSFER_QUEUE_MAX_ITEMS 12

#define RENDER_PIPELINE_MAX_STAGES 8
#define RENDER_STAGE_MAX_RESOURCES 4

#define CLEAR_COLOR (SDL_FColor){0.54, 0.9, 0.64, 1.0f}
#define INDEX_SIZE SDL_GPU_INDEXELEMENTSIZE_16BIT

enum GPUTransferType { GPU_TRANSFER_BUFFER, GPU_TRANSFER_TEXTURE };

//
// GPU Transfer Queue:
// Move resources to the GPU
struct GPUTransferQueueItem {
  void *data;
  size_t size;
  enum GPUTransferType type;
  union {
    SDL_GPUBuffer *buffer;
    struct {
      uint32_t w;
      uint32_t h;
      SDL_GPUTexture *texture;
    } texture;
  } dest;
};
struct GPUTransferQueue {
  struct GPUTransferQueueItem items[GPU_TRANSFER_QUEUE_MAX_ITEMS];
  size_t front;
  size_t back;
  size_t size;
  SDL_GPUCopyPass *copy;
};
bool EnqueueTransferToGPUBuffer(struct GPUTransferQueue *queue, void *data,
                                size_t size, SDL_GPUBuffer *dest) {
  if (queue->size == GPU_TRANSFER_QUEUE_MAX_ITEMS) {
    return false;
  }

  queue->back = (queue->back + 1) % GPU_TRANSFER_QUEUE_MAX_ITEMS;
  queue->size++;
  queue->items[queue->back] =
      (struct GPUTransferQueueItem){.data = data,
                                    .size = size,
                                    .type = GPU_TRANSFER_BUFFER,
                                    .dest.buffer = dest};

  return true;
}
bool EnqueueTransferToGPUTexture(struct GPUTransferQueue *queue, void *data,
                                 size_t size, uint32_t w, uint32_t h,
                                 SDL_GPUTexture *texture) {
  if (queue->size == GPU_TRANSFER_QUEUE_MAX_ITEMS) {
    return false;
  }

  queue->back = (queue->back + 1) % GPU_TRANSFER_QUEUE_MAX_ITEMS;
  queue->size++;
  queue->items[queue->back] = (struct GPUTransferQueueItem){
      .data = data,
      .size = size,
      .type = GPU_TRANSFER_TEXTURE,
      .dest.texture = {.w = w, .h = h, .texture = texture},
  };

  return true;
}
// WARNING: Do not call this function outside of a GPU copy pass
bool DequeueGPUTransferItem(struct GPUTransferQueue *queue,
                            SDL_GPUDevice *device) {
  if (queue->size == 0) {
    return false;
  }
  assert(queue->copy);

  struct GPUTransferQueueItem item = queue->items[queue->front];
  queue->front = (queue->front + 1) % GPU_TRANSFER_QUEUE_MAX_ITEMS;
  queue->size--;

  SDL_GPUTransferBuffer *buffer = SDL_CreateGPUTransferBuffer(
      device, &(SDL_GPUTransferBufferCreateInfo){
                  .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                  .size = item.size,
              });
  if (!buffer) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create transfer buffer: %s\n",
                 SDL_GetError());
    return false;
  }

  void *transfer = SDL_MapGPUTransferBuffer(device, buffer, false);
  if (!transfer) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Map transfer buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_memcpy(transfer, item.data, item.size);
  SDL_UnmapGPUTransferBuffer(device, buffer);

  switch (item.type) {
  case GPU_TRANSFER_BUFFER:
    SDL_UploadToGPUBuffer(
        queue->copy,
        &(SDL_GPUTransferBufferLocation){.transfer_buffer = buffer},
        &(SDL_GPUBufferRegion){.buffer = item.dest.buffer, .size = item.size},
        false);
    break;
  case GPU_TRANSFER_TEXTURE:
    SDL_UploadToGPUTexture(queue->copy,
                           &(SDL_GPUTextureTransferInfo){
                               .transfer_buffer = buffer,
                               .pixels_per_row = item.dest.texture.w,
                               .rows_per_layer = item.dest.texture.h,
                           },
                           &(SDL_GPUTextureRegion){
                               .texture = item.dest.texture.texture,
                               .w = item.dest.texture.w,
                               .h = item.dest.texture.h,
                               .mip_level = 1,
                               .layer = 1,
                               .d = 1,
                           },
                           false);
    break;
  }

  SDL_ReleaseGPUTransferBuffer(device, buffer);

  return true;
}
bool EmptyGPUTransferQueue(struct GPUTransferQueue *queue,
                           SDL_GPUDevice *device) {
  SDL_GPUCommandBuffer *cmdBuf = SDL_AcquireGPUCommandBuffer(device);
  if (!cmdBuf) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Acquire GPU command buffer: %s\n",
                 SDL_GetError());
    return false;
  }
  queue->copy = SDL_BeginGPUCopyPass(cmdBuf);

  while (queue->size > 0) {
    if (!DequeueGPUTransferItem(queue, device)) {
      return false;
    }
  }

  SDL_EndGPUCopyPass(queue->copy);
  queue->copy = NULL;

  SDL_SubmitGPUCommandBuffer(cmdBuf);

  return true;
}

//
// Staged Render Pipeline
// Queue multiple rendering steps to composite a final image
typedef enum {
  RENDER_RESOURCE_TYPE_VERTEX_BUFFER,
  RENDER_RESOURCE_TYPE_INDEX_BUFFER,
  RENDER_RESOURCE_TYPE_TEXTURE_SAMPLER_PAIR,
} RenderResourceType;

typedef struct {
  uint32_t type;
  size_t size;
  union {
    SDL_GPUBufferCreateInfo vertex;
    SDL_GPUBufferCreateInfo index;
    struct {
      SDL_GPUTextureCreateInfo texture;
      SDL_GPUSamplerCreateInfo sampler;
    } textureSamplerPair;
    struct {
      SDL_GPUShaderCreateInfo shader;
    } renderProgram;
  } info;
} RenderResource;

typedef enum {
  RENDER_SLOT_TYPE_VERTEX,
  RENDER_SLOT_TYPE_INDEX,
  RENDER_SLOT_TYPE_TEXTURE_SAMPLER_PAIR,
} RenderSlotType;
typedef struct {
  RenderSlotType type;
  uint32_t binding;
} RenderProgramSlot;

typedef struct {
  SDL_GPUShaderCreateInfo info;
  union {
    struct {
    } none;
    SDL_GPUVertexInputState vertex;
  } state;
} RenderProgram;

typedef struct {
  SDL_GPUPrimitiveType primitive_type;
  // SDL_GPURasterizerState rasterizer_state;
  // SDL_GPUMultisampleState multisample_state;
  // SDL_GPUDepthStencilState depth_stencil_state;
  SDL_GPUGraphicsPipelineTargetInfo target_info;
} RenderStageInfo;

typedef struct {
  RenderStageInfo info;
  RenderProgram vertexProgram;
  RenderProgram fragmentProgram;
  RenderResource resources[RENDER_STAGE_MAX_RESOURCES];
  RenderProgramSlot bindings[RENDER_STAGE_MAX_RESOURCES];
  size_t resourceCount;
  struct {
    SDL_GPUShader *vertexProgram;
    SDL_GPUShader *FragmentProgram;
    SDL_GPUBuffer *vertexBuf;
    SDL_GPUBuffer *indexBuf;
    SDL_GPUTexture *textureBuf;
    SDL_GPUSampler *sampler;
    SDL_GPUGraphicsPipeline *pipeline;
  } running;
} DrawPipelineStage;

DrawPipelineStage DrawPipelineStage_init(void) {
  DrawPipelineStage newStage = {0};
  return newStage;
}
void RenderStage_WithInfo(DrawPipelineStage *self, RenderStageInfo info) {
  assert(self);

  self->info = info;
}
void RenderStage_WithProgram(DrawPipelineStage *self, RenderProgram program) {
  assert(self);

  switch (program.info.stage) {
  case SDL_GPU_SHADERSTAGE_VERTEX:
    self->vertexProgram = program;
    break;
  case SDL_GPU_SHADERSTAGE_FRAGMENT:
    self->fragmentProgram = program;
    break;
  }
}
void RenderStage_WithResource(DrawPipelineStage *self, RenderResource resource,
                              RenderProgramSlot slot) {
  assert(self);
  assert(self->resourceCount + 1 < RENDER_STAGE_MAX_RESOURCES);

  self->resources[self->resourceCount] = resource;
  self->bindings[self->resourceCount] = slot;
  self->resourceCount += 1;
}
SDL_AppResult RenderStage_Build(DrawPipelineStage *self,
                                SDL_GPUDevice *device) {
  // Construct resources
  for (size_t i = 0; i < self->resourceCount; i += 1) {
    RenderResource resource = self->resources[i];

    // Construct buffers
    if (resource.type == RENDER_RESOURCE_TYPE_VERTEX_BUFFER) {
      SDL_GPUBuffer *vertex =
          SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo){
                                          .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                          .size = resource.size,
                                      });
      if (!vertex) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create vertex buffer: %s\n",
                     SDL_GetError());
        return SDL_APP_FAILURE;
      }
      SDL_SetGPUBufferName(device, vertex, "vertex");
      self->running.vertexBuf = vertex;
    } else if (RENDER_RESOURCE_TYPE_INDEX_BUFFER) {
      SDL_GPUBuffer *index = SDL_CreateGPUBuffer(
          device,
          &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                     .size = resource.size});
      if (!index) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create vertex buffer: %s\n",
                     SDL_GetError());
        return SDL_APP_FAILURE;
      }
      SDL_SetGPUBufferName(device, index, "index");
      self->running.indexBuf = index;
    } else if (resource.type == RENDER_RESOURCE_TYPE_TEXTURE_SAMPLER_PAIR) {
      SDL_GPUTexture *texture = SDL_CreateGPUTexture(
          device, &(SDL_GPUTextureCreateInfo){
                      .type = SDL_GPU_TEXTURETYPE_2D,
                      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                      .width = resource.info.textureSamplerPair.texture.width,
                      .height = resource.info.textureSamplerPair.texture.height,
                      .layer_count_or_depth = 1,
                      .num_levels = 1,
                  });
      if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create texture: %s\n",
                     SDL_GetError());
        return SDL_APP_FAILURE;
      }
      SDL_SetGPUTextureName(device, texture, "texture");
      self->running.textureBuf = texture;

      SDL_GPUSampler *sampler = SDL_CreateGPUSampler(
          device,
          &(SDL_GPUSamplerCreateInfo){
              .min_filter = SDL_GPU_FILTER_NEAREST,
              .mag_filter = SDL_GPU_FILTER_NEAREST,
              .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
              .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
              .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
          });
      if (!sampler) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create sampler: %s\n",
                     SDL_GetError());
        return SDL_APP_FAILURE;
      }
      self->running.sampler = sampler;
    }
  }

  // Construct shaders
  SDL_GPUShader *vShader =
      SDL_CreateGPUShader(device, &self->vertexProgram.info);
  if (!vShader) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create fragment shader: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  self->running.vertexProgram = vShader;

  SDL_GPUShader *fShader =
      SDL_CreateGPUShader(device, &self->fragmentProgram.info);
  if (!vShader) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create fragment shader: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  self->running.FragmentProgram = fShader;

  // Construct Pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
      .vertex_shader = self->running.vertexProgram,
      .fragment_shader = self->running.FragmentProgram,
      .vertex_input_state = self->vertexProgram.state.vertex,
      .primitive_type = self->info.primitive_type,
      .target_info = self->info.target_info,
  };
  SDL_GPUGraphicsPipeline *pipeline =
      SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
  if (!pipeline) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create pipeline: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  self->running.pipeline = pipeline;

  return SDL_APP_CONTINUE;
}

SDL_AppResult RenderStage_Run(DrawPipelineStage *self,
                              SDL_GPUCommandBuffer *cmdBuf,
                              SDL_Window *window) {
  // NOTE: Swapchain acquisition is a gate to begin a render pass, but our app
  // may continue successfully without producing a render pass on this
  // iteration.
  SDL_GPUTexture *swapchain;
  if (!SDL_AcquireGPUSwapchainTexture(cmdBuf, window, &swapchain, NULL, NULL)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Acquire swapchain: %s",
                 SDL_GetError());
  }
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Acquired GPU swap chain texture");
  if (!swapchain) {
    SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "No swapchain texture acquired");
    return SDL_APP_CONTINUE;
  }

  // SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Push GPU uniform vertex data");
  // SDL_PushGPUVertexUniformData(cmdBuf, 0, mvp, sizeof(mvp));

  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "SDL render pass begin");
  SDL_GPURenderPass *pass =
      SDL_BeginGPURenderPass(cmdBuf,
                             (SDL_GPUColorTargetInfo[]){{
                                 .texture = swapchain,
                                 .clear_color = CLEAR_COLOR,
                                 .load_op = SDL_GPU_LOADOP_CLEAR,
                                 .store_op = SDL_GPU_STOREOP_STORE,
                             }},
                             1, NULL);

  // Render - Binding
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Bind pipeline");
  SDL_BindGPUGraphicsPipeline(pass, self->running.pipeline);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Bind vertex buffer");
  int quadVertexBuf = 0;
  SDL_BindGPUVertexBuffers(pass, quadVertexBuf,
                           (SDL_GPUBufferBinding[]){{
                               .buffer = self->running.vertexBuf,
                           }},
                           1);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Bind index buffer");
  SDL_BindGPUIndexBuffer(
      pass, &(SDL_GPUBufferBinding){.buffer = self->running.indexBuf},
      INDEX_SIZE);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Bind atlas");
  assert(self->running.textureBuf);
  assert(self->running.sampler);
  int atlasTexBuf = 0;
  SDL_BindGPUFragmentSamplers(pass, atlasTexBuf,
                              (SDL_GPUTextureSamplerBinding[]){{
                                  .texture = self->running.textureBuf,
                                  .sampler = self->running.sampler,
                              }},
                              1);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Binding complete");

  // SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Draw quads: %zu quads",
  // quadBuf.count); uint32_t numIndices = QUAD_INDEX_COUNT * quadBuf.count;
  // SDL_DrawGPUIndexedPrimitives(pass, numIndices, 1, 0, 0, 0);

  SDL_EndGPURenderPass(pass);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "SDL render pass complete");

  return SDL_APP_CONTINUE;
}

typedef struct {
  DrawPipelineStage stages[RENDER_PIPELINE_MAX_STAGES];
  size_t stageCount;
} DrawPipeline;

DrawPipeline RenderPipeline_Init(void) {
  DrawPipeline newPipeline = {0};
  return newPipeline;
}
void RenderPipeline_WithStage(DrawPipeline *self, DrawPipelineStage stage) {
  assert(self);
  assert(self->stageCount + 1 < RENDER_PIPELINE_MAX_STAGES);

  self->stages[self->stageCount] = stage;
  self->stageCount += 1;
}
void RenderPipeline_Build(DrawPipeline *self, SDL_GPUDevice *device) {
  for (size_t stage = 0; stage < self->stageCount; stage++) {
    RenderStage_Build(&self->stages[stage], device);
  }
}
void RenderPipeline_Run(DrawPipeline *self, SDL_GPUCommandBuffer *cmdBuf,
                        SDL_Window *window) {
  for (size_t stage = 0; stage < self->stageCount; stage++) {
    RenderStage_Run(&self->stages[stage], cmdBuf, window);
  }
}

// TODO: An interface to run the pipeline after it has been built
