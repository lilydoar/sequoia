#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"

#include "cglm/mat4.h"
#include "cglm/types.h"
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_W 800
#define WINDOW_H 600

struct Context {
  SDL_GPUDevice *device;
  SDL_Window *window;
};

bool initWindow(struct Context *context) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Initialize SDL: %s\n",
                 SDL_GetError());
    return false;
  }

  SDL_GPUDevice *device =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, NULL);
  if (device == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create GPU device: %s\n",
                 SDL_GetError());
    SDL_Quit();
    return false;
  }

  SDL_Window *window = SDL_CreateWindow(NULL, WINDOW_W, WINDOW_H, 0);
  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create window: %s\n", SDL_GetError());
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    return false;
  }

  if (!SDL_ClaimWindowForGPUDevice(device, window)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Claim window for GPU: %s\n",
                 SDL_GetError());
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyWindow(window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    return false;
  }

  struct Context c = {
      .device = device,
      .window = window,
  };
  *context = c;

  return true;
}

void deinitWindow(struct Context context) {
  SDL_ReleaseWindowFromGPUDevice(context.device, context.window);
  SDL_DestroyWindow(context.window);
  SDL_DestroyGPUDevice(context.device);
  SDL_Quit();
}

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
      .num_samplers = 1,
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
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Loaded shader %s\n", file);

  SDL_free(contents);
  return shader;
}

SDL_Surface *loadImage(const char *file) {
  int width, height, channels;
  unsigned char *pixels = stbi_load(file, &width, &height, &channels, 0);
  if (pixels == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Image load failed: %s\n",
                 stbi_failure_reason());
    return NULL;
  }
  SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "Loaded image %s: %dx%dx%d\n", file,
               width, height, channels);

  SDL_Surface *surface = SDL_CreateSurfaceFrom(
      width, height, SDL_PIXELFORMAT_RGBA8888, pixels, width * channels);
  if (surface == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Surface creation failed: %s\n",
                 SDL_GetError());
    return NULL;
  }

  return surface;
}

void deinitImage(SDL_Surface *surface) {
  void *pixels = surface->pixels;
  SDL_DestroySurface(surface);
  stbi_image_free(pixels);
}

struct ColorVertex {
  vec3 position;
  vec3 color;
};

struct TextureVertex {
  vec3 position;
  vec2 uv;
};

struct Texture {};
struct Sprite {};

struct Quad {
  vec2 p1;
  vec2 p2;
};

struct SEQ_RenderQueue {};
struct SEQ_Bucket {};
enum SEQ_LayerType {
  SEQ_LAYER_TYPE_EMPTY,
  SEQ_LAYER_TYPE_TILEMAP,
  SEQ_LAYER_TYPE_ENTITY,
  SEQ_LAYER_TYPE_UI,
};
struct SEQ_Layer {
  enum SEQ_LayerType type;
};

/// The QueueUpload commmands prepare some data to be moved to the GPU.
/// This holds onto SDL Transfer buffers and creates the relevant upload
/// commands
void SEQ_QueueUploadVertices(vec2 *vertices);
void SEQ_QueueUploadVerticesWithIndices(vec2 *vertices, size_t indices);
void SEQ_QueueUploadTexture(struct Texture *texure, size_t indices);
/// Empties all queued GPU Transfer commands.
/// This starts the move of the data to the GPU.
void SEQ_EmptyGPUTransferQueue();

struct Rect {};

struct Tile {
  struct Rect textureRect;
};
struct Tilemap {
  struct Texture *textureAtlas;
  // Map tile types to a name
  char **names;
  // Map tile types to a texture
  struct Rect *textures;

  uint32_t scale;
  uint32_t width;
  uint32_t height;
  /// Warning: Only supports up to 255 tile types
  uint8_t *tiles;
};
struct UI {};

void SEQ_DrawTileMap(struct SEQ_RenderQueue rQueue, struct Tilemap *tMap);
void SEQ_DrawUI(struct SEQ_RenderQueue rQueue, struct UI *ui);

SDL_GPUGraphicsPipeline *createPipeline(struct Context *context,
                                        SDL_GPUShader *vertexShader,
                                        SDL_GPUShader *fragmentShader) {
  // FIXME: Pipeline is hardcoded to texture vertices
  SDL_GPUVertexBufferDescription vertexBufferDesc[] = {{
      .slot = 0,
      .pitch = sizeof(struct TextureVertex),
      .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
      .instance_step_rate = 0,
  }};
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
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = sizeof(vec3),
      },
  };
  SDL_GPUColorTargetDescription colorTarget = {
      .format =
          SDL_GetGPUSwapchainTextureFormat(context->device, context->window),
      .blend_state =
          {
              .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
              .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
              .color_blend_op = SDL_GPU_BLENDOP_ADD,
              .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
              .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
              .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
              .enable_blend = true,
              .enable_color_write_mask = false,
          },
  };
  SDL_GPUGraphicsPipelineCreateInfo info = {
      .vertex_shader = vertexShader,
      .fragment_shader = fragmentShader,
      .vertex_input_state =
          {
              .vertex_buffer_descriptions = vertexBufferDesc,
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
      SDL_CreateGPUGraphicsPipeline(context->device, &info);
  if (pipeline == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Graphics pipeline creation failed: %s\n", SDL_GetError());
    return NULL;
  }

  return pipeline;
}

int main(void) {
  SDL_SetLogPriorities(SDL_LOG_PRIORITY_DEBUG);

  struct Context context;
  if (!initWindow(&context)) {
    return 1;
  }

  const char *file = "shaders/metal/textured_quad.metal";
  SDL_GPUShader *vertex = loadShader(context.device, file, "vertexMain",
                                     SDL_GPU_SHADERSTAGE_VERTEX);
  if (vertex == NULL) {
    return 1;
  }
  SDL_GPUShader *fragment = loadShader(context.device, file, "fragmentMain",
                                       SDL_GPU_SHADERSTAGE_FRAGMENT);
  if (fragment == NULL) {
    return 1;
  }

  const char *textureFile =
      "assets/sprites/Factions/Knights/Buildings/Castle/Castle_Blue.png";
  SDL_Surface *surface = loadImage(textureFile);
  if (surface == NULL) {
    return 1;
  }

  SDL_GPUGraphicsPipeline *pipeline =
      createPipeline(&context, vertex, fragment);
  if (pipeline == NULL) {
    return 1;
  }

  SDL_ReleaseGPUShader(context.device, vertex);
  SDL_ReleaseGPUShader(context.device, fragment);

  /*struct ColorVertex triangleVertices[3] = {*/
  /*    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Bottom left (red)*/
  /*    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // Bottom right (green)*/
  /*    {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}    // Top (blue)*/
  /*};*/

  struct TextureVertex quadVertices[6] = {
      {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}}, // T1 Bottom left
      {{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}},  // T1 Bottom right
      {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f}},  // T1 Top left
      {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f}},   // T2 Top Right
      {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f}},  // T2 Top left
      {{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}},  // T2 Bottom right
  };

  SDL_GPUTransferBufferCreateInfo vertexTransferBufferInfo = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = sizeof(quadVertices),
      .props = 0,
  };
  SDL_GPUTransferBuffer *vertexTransferBuffer =
      SDL_CreateGPUTransferBuffer(context.device, &vertexTransferBufferInfo);

  struct ColorVertex *vertexTransferData =
      SDL_MapGPUTransferBuffer(context.device, vertexTransferBuffer, false);
  if (vertexTransferData == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Mapping GPU transfer buffer failed: %s\n", SDL_GetError());
    return 1;
  }

  memcpy(vertexTransferData, quadVertices, sizeof(quadVertices));

  SDL_UnmapGPUTransferBuffer(context.device, vertexTransferBuffer);

  SDL_GPUBufferCreateInfo vertexBufferInfo = {
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
      .size = sizeof(quadVertices),
      .props = 0,
  };
  SDL_GPUBuffer *vertexBuffer =
      SDL_CreateGPUBuffer(context.device, &vertexBufferInfo);
  if (vertexBuffer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vertex buffer creation: %s\n",
                 SDL_GetError());
    return 1;
  }
  SDL_SetGPUBufferName(context.device, vertexBuffer, "Vertex Buffer");

  SDL_GPUTransferBufferCreateInfo textureTransferBufferInfo = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = surface->w * surface->h * 4,
      .props = 0,
  };
  SDL_GPUTransferBuffer *textureTransferBuffer =
      SDL_CreateGPUTransferBuffer(context.device, &textureTransferBufferInfo);

  uint8_t *textureTransferData =
      SDL_MapGPUTransferBuffer(context.device, textureTransferBuffer, false);
  if (textureTransferData == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Mapping GPU transfer buffer failed: %s\n", SDL_GetError());
    return 1;
  }

  memcpy(textureTransferData, surface->pixels, surface->w * surface->h * 4);

  SDL_UnmapGPUTransferBuffer(context.device, textureTransferBuffer);

  SDL_GPUTextureCreateInfo textureInfo = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = surface->w,
      .height = surface->h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
  };
  SDL_GPUTexture *texture = SDL_CreateGPUTexture(context.device, &textureInfo);
  if (texture == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Texture creation: %s\n",
                 SDL_GetError());
    return 1;
  }
  SDL_SetGPUTextureName(context.device, texture, textureFile);

  SDL_GPUSamplerCreateInfo samplerInfo = {
      .min_filter = SDL_GPU_FILTER_NEAREST,
      .mag_filter = SDL_GPU_FILTER_NEAREST,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };
  SDL_GPUSampler *sampler = SDL_CreateGPUSampler(context.device, &samplerInfo);
  if (sampler == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Sampler creation: %s\n",
                 SDL_GetError());
    return 1;
  }

  mat4 modelViewProjectionMatrix = GLM_MAT4_IDENTITY;

  SDL_GPUTransferBufferCreateInfo uniformTransferBufferInfo = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = sizeof(modelViewProjectionMatrix),
      .props = 0,
  };
  SDL_GPUTransferBuffer *uniformTransferBuffer =
      SDL_CreateGPUTransferBuffer(context.device, &uniformTransferBufferInfo);

  mat4 *uniformTransferData =
      SDL_MapGPUTransferBuffer(context.device, uniformTransferBuffer, false);
  if (uniformTransferData == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Mapping GPU transfer buffer failed: %s\n", SDL_GetError());
    return 1;
  }

  memcpy(uniformTransferData, modelViewProjectionMatrix,
         sizeof(modelViewProjectionMatrix));

  SDL_UnmapGPUTransferBuffer(context.device, uniformTransferBuffer);

  SDL_GPUBufferCreateInfo uniformBufferInfo = {
      .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
      .size = sizeof(modelViewProjectionMatrix),
      .props = 0,
  };
  SDL_GPUBuffer *uniformBuffer =
      SDL_CreateGPUBuffer(context.device, &uniformBufferInfo);
  if (uniformBuffer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Uniform buffer creation failed: %s\n",
                 SDL_GetError());
    return 1;
  }
  SDL_SetGPUBufferName(context.device, uniformBuffer, "Uniform Buffer");

  SDL_GPUCommandBuffer *uploadCommandBuffer =
      SDL_AcquireGPUCommandBuffer(context.device);
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
      .size = sizeof(quadVertices),
  };
  SDL_UploadToGPUBuffer(copyPass, &vertexSource, &vertexDest, false);

  SDL_GPUTextureTransferInfo textureSource = {
      .transfer_buffer = textureTransferBuffer,
      .offset = 0,
      .pixels_per_row = surface->w,
      .rows_per_layer = surface->h,
  };
  SDL_GPUTextureRegion textureDest = {
      .texture = texture,
      .mip_level = 1,
      .layer = 1,
      .x = 0,
      .y = 0,
      .z = 0,
      .w = surface->w,
      .h = surface->h,
      .d = 1,
  };
  SDL_UploadToGPUTexture(copyPass, &textureSource, &textureDest, false);

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

  SDL_ReleaseGPUTransferBuffer(context.device, vertexTransferBuffer);
  SDL_ReleaseGPUTransferBuffer(context.device, textureTransferBuffer);
  SDL_ReleaseGPUTransferBuffer(context.device, uniformTransferBuffer);

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
        SDL_AcquireGPUCommandBuffer(context.device);
    if (renderCommandBuffer == NULL) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Failed to acquire GPU command buffer: %s", SDL_GetError());
      break;
    }

    SDL_GPUTexture *swapchainTexture;
    if (!SDL_AcquireGPUSwapchainTexture(renderCommandBuffer, context.window,
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
    SDL_GPUBufferBinding vertexBinding = {
        .buffer = vertexBuffer,
        .offset = 0,
    };
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);
    SDL_GPUTextureSamplerBinding textureBinding = {
        .texture = texture,
        .sampler = sampler,
    };
    SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBinding, 1);

    SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);

    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(renderCommandBuffer);
  }

  SDL_ReleaseGPUGraphicsPipeline(context.device, pipeline);
  SDL_ReleaseGPUBuffer(context.device, vertexBuffer);
  SDL_ReleaseGPUBuffer(context.device, uniformBuffer);

  deinitImage(surface);
  deinitWindow(context);

  return 0;
}
