#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_log.h"

#define GPU_TRANSFER_QUEUE_MAX_ITEMS 12

#define RENDER_PIPELINE_MAX_STAGES 8
#define RENDER_STAGE_MAX_RESOURCES 4

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
} RenderStage;

RenderStage RenderStage_Init(void) {
  RenderStage newStage = {0};
  return newStage;
}
void RenderStage_WithInfo(RenderStage *self, RenderStageInfo info) {
  assert(self);

  self->info = info;
}
void RenderStage_WithProgram(RenderStage *self, RenderProgram program) {
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
void RenderStage_WithResource(RenderStage *self, RenderResource resource,
                              RenderProgramSlot slot) {
  assert(self);
  assert(self->resourceCount + 1 < RENDER_STAGE_MAX_RESOURCES);

  self->resources[self->resourceCount] = resource;
  self->bindings[self->resourceCount] = slot;
  self->resourceCount += 1;
}
void RenderStage_Build(RenderStage *self) {
  // TODO
  // Construct buffers, shaders, and an SDL pipeline
}

typedef struct {
  RenderStage stages[RENDER_PIPELINE_MAX_STAGES];
  size_t stageCount;
} RenderPipeline;

RenderPipeline RenderPipeline_Init(void) {
  RenderPipeline newPipeline = {0};
  return newPipeline;
}
void RenderPipeline_WithStage(RenderPipeline *self, RenderStage stage) {
  assert(self);
  assert(self->stageCount + 1 < RENDER_PIPELINE_MAX_STAGES);

  self->stages[self->stageCount] = stage;
  self->stageCount += 1;
}
void RenderPipeline_Build(RenderPipeline *self) {
  for (size_t stage = 0; stage < self->stageCount; stage++) {
    RenderStage_Build(&self->stages[stage]);
  }
}

// TODO: An interface to run the pipeline after it has been built
