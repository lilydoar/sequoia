#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"

// clang-format off
#include "cglm/affine.h"
#include "cglm/affine-pre.h" // clang-format on
#include "cglm/cam.h"
#include "cglm/mat4.h"
#include "cglm/types.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAX_QUADS 1024
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)
#define GPU_TRANSFER_QUEUE_MAX 8

#define CLEAR_COLOR (SDL_FColor){0.3f, 0.4f, 0.5f, 1.0f}
#define INDEX_SIZE SDL_GPU_INDEXELEMENTSIZE_16BIT

#define QUAD_VERTEX_COUNT 4
#define QUAD_INDEX_COUNT 6
static float QUAD_VERTEX_POSITIONS[] = {-0.5, 0.5,  -0.5, -0.5,
                                        0.5,  -0.5, 0.5,  0.5};
static float QUAD_UV[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0};

struct App {
  const char *name;
  const char *version;
  const char *identifier;
};

struct Window {
  const char *title;
  uint16_t width;
  uint16_t height;
  SDL_WindowFlags flags;
  SDL_Window *window;
};

struct Atlas {
  const char *path;
  // FIXME: Should be uint16_t
  int w;
  int h;
  // FIXME: Should be uint8_t
  int channels;
  unsigned char *pixels;
  SDL_Surface *surface;
  SDL_GPUTexture *texture;
  SDL_GPUSampler *sampler;
};
size_t AtlasSize(struct Atlas self) { return self.w * self.h * self.channels; }

enum GPUTransferType { GPU_TRANSFER_BUFFER, GPU_TRANSFER_TEXTURE };
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
  struct GPUTransferQueueItem items[GPU_TRANSFER_QUEUE_MAX];
  size_t front;
  size_t back;
  size_t size;
  SDL_GPUCopyPass *copy;
};
bool EnqueueTransferToGPUBuffer(struct GPUTransferQueue *queue, void *data,
                                size_t size, SDL_GPUBuffer *dest) {
  if (queue->size == GPU_TRANSFER_QUEUE_MAX) {
    return false;
  }

  queue->back = (queue->back + 1) % GPU_TRANSFER_QUEUE_MAX;
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
  if (queue->size == GPU_TRANSFER_QUEUE_MAX) {
    return false;
  }

  queue->back = (queue->back + 1) % GPU_TRANSFER_QUEUE_MAX;
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
  queue->front = (queue->front + 1) % GPU_TRANSFER_QUEUE_MAX;
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

struct Render {
  SDL_GPUShaderFormat supported;
  SDL_GPUShaderFormat shader;
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
  struct GPUTransferQueue queue;
  struct Atlas atlas;
  SDL_GPUBuffer *vertex;
  SDL_GPUBuffer *index;
};

struct Camera {
  vec2 position;
  vec2 size;
  float scale;
};
void camera_model_view_proj(struct Camera camera, mat4 mvp) {
  float scaled_width = camera.size[0] * camera.scale / 2;
  float scaled_height = camera.size[1] * camera.scale / 2;

  mat4 projection;
  glm_ortho(scaled_width, -scaled_width, -scaled_height, scaled_height, 0.0,
            1000.0, projection);

  mat4 view;
  glm_mat4_identity(view);
  glm_translate(view, (vec3){camera.position[0], -camera.position[1], 0.0});

  glm_mat4_mul(projection, view, mvp);
}

struct Fire {
  vec2 position;
  vec2 size;
};

struct Game {
  struct Camera camera;
  struct Fire fire;
};

struct Context {
  struct App app;
  struct Window window;
  struct Render render;
  struct Game game;
};

struct Vertex {
  vec2 position;
  vec2 uv;
};

char *format_mat4(const mat4 m) {
  // WARNING: This function is not thread safe because of this static buffer
  static char buffer[256];
  int offset = 0;
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      offset += sprintf(buffer + offset, "%.3f ", m[col][row]);
    }
    offset += sprintf(buffer + offset, "\n");
  }
  return buffer;
}

const char *format_quad(const struct Vertex *vertices) {
  // WARNING: This function is not thread safe because of this static buffer
  static char buffer[512];
  int offset = 0;

  offset += sprintf(buffer + offset, "Quad:\n");
  for (int i = 0; i < 4; i++) {
    offset += sprintf(buffer + offset, "v%d: pos(%.3f, %.3f) uv(%.3f, %.3f)\n",
                      i, vertices[i].position[0], vertices[i].position[1],
                      vertices[i].uv[0], vertices[i].uv[1]);
  }

  return buffer;
}

const char *format_transformed_quad(const struct Vertex *vertices, mat4 mvp) {
  static char buffer[1024];
  int offset = 0;

  offset += sprintf(buffer + offset, "Original Vertices:\n");
  offset += sprintf(buffer + offset, "%s\n", format_quad(vertices));

  offset += sprintf(buffer + offset, "MVP Matrix:\n%s\n", format_mat4(mvp));

  offset += sprintf(buffer + offset, "Transformed Positions:\n");
  for (int i = 0; i < 4; i++) {
    // Create homogeneous coordinate (x,y,z,w)
    vec4 pos = {vertices[i].position[0], vertices[i].position[1], 0.0f, 1.0f};
    vec4 transformed;

    // Transform vertex by MVP
    glm_mat4_mulv(mvp, pos, transformed);

    // Perform perspective divide if w != 1
    if (transformed[3] != 1.0f && transformed[3] != 0.0f) {
      transformed[0] /= transformed[3];
      transformed[1] /= transformed[3];
      transformed[2] /= transformed[3];
    }

    offset +=
        sprintf(buffer + offset, "v%d: (%.3f, %.3f, %.3f, %.3f)\n", i,
                transformed[0], transformed[1], transformed[2], transformed[3]);
  }

  return buffer;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);

  struct Context *context = SDL_malloc(sizeof(struct Context));

  // App
  struct App app = {
      .name = "Sequioa",
      .version = "0.0.1",
      .identifier = "com.lilydoar.sequioa",
  };
  if (!SDL_SetAppMetadata(app.name, app.version, app.identifier)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Set app metadata: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  context->app = app;

  // Window
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Init video: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  struct Window window = {
      .title = "Sequioa",
      .width = 800,
      .height = 600,
  };
  window.window =
      SDL_CreateWindow(window.title, window.width, window.height, window.flags);
  if (!window.window) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create window: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  context->window = window;

  // Static Data

  // Index data for quads can be precomputed
  uint16_t indices[MAX_INDICES];
  for (size_t index = 0; index < MAX_INDICES; index += QUAD_INDEX_COUNT) {
    indices[index + 0] = index / 6 * 4 + 0;
    indices[index + 1] = index / 6 * 4 + 1;
    indices[index + 2] = index / 6 * 4 + 2;
    indices[index + 3] = index / 6 * 4 + 0;
    indices[index + 4] = index / 6 * 4 + 2;
    indices[index + 5] = index / 6 * 4 + 3;
  }

  // Render
  struct Render render = {
      .supported = SDL_GPU_SHADERFORMAT_MSL,
  };
  render.device = SDL_CreateGPUDevice(render.supported, true, NULL);
  if (!render.device) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create GPU device: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_ClaimWindowForGPUDevice(render.device, context->window.window)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Claim window for GPU: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Render - Shader
  SDL_GPUShaderFormat deviceFormats = SDL_GetGPUShaderFormats(render.device);

  const char *shaderPath;
  if (deviceFormats & SDL_GPU_SHADERFORMAT_MSL) {
    render.shader = SDL_GPU_SHADERFORMAT_MSL;
    shaderPath = "shaders/metal/sprite.metal";
  } else {
    return SDL_APP_FAILURE;
  }

  size_t fileSize;
  void *contents = SDL_LoadFile(shaderPath, &fileSize);
  if (!contents) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Load shader file: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUShaderCreateInfo vertexInfo = {
      .code_size = fileSize,
      .code = contents,
      .entrypoint = "vertexMain",
      .format = render.shader,
      .stage = SDL_GPU_SHADERSTAGE_VERTEX,
      .num_uniform_buffers = 1,
  };
  SDL_GPUShader *vertex = SDL_CreateGPUShader(render.device, &vertexInfo);
  if (!vertex) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create vertex shader: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUShaderCreateInfo fragmentInfo = {
      .code_size = fileSize,
      .code = contents,
      .entrypoint = "fragmentMain",
      .format = render.shader,
      .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
      .num_samplers = 1,
  };
  SDL_GPUShader *fragment = SDL_CreateGPUShader(render.device, &fragmentInfo);
  if (!fragment) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create fragment shader: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "Shader load complete: %s\n", shaderPath);

  SDL_free(contents);

  // Render - Pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
      .vertex_shader = vertex,
      .fragment_shader = fragment,
      .vertex_input_state =
          {
              .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
                  .slot = 0,
                  .pitch = sizeof(struct Vertex),
                  .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
              }},
              .num_vertex_buffers = 1,
              .vertex_attributes =
                  (SDL_GPUVertexAttribute[]){
                      {
                          .location = 0,
                          .buffer_slot = 0,
                          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                          .offset = 0,
                      },
                      {
                          .location = 1,
                          .buffer_slot = 0,
                          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                          .offset = sizeof(vec2),
                      }},
              .num_vertex_attributes = 2,
          },
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .target_info =
          {
              .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                  .format = SDL_GetGPUSwapchainTextureFormat(render.device,
                                                             window.window),
                  .blend_state =
                      {
                          .enable_blend = true,
                          .color_blend_op = SDL_GPU_BLENDOP_ADD,
                          .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                          .src_color_blendfactor =
                              SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                          .src_alpha_blendfactor =
                              SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                          .dst_color_blendfactor =
                              SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                          .dst_alpha_blendfactor =
                              SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                      },
              }},
              .num_color_targets = 1,
          },
  };
  render.pipeline = SDL_CreateGPUGraphicsPipeline(render.device, &pipelineInfo);
  if (!render.pipeline) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create pipeline: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_ReleaseGPUShader(render.device, vertex);
  SDL_ReleaseGPUShader(render.device, fragment);

  // Render - GPU Transfer Queue
  render.queue = (struct GPUTransferQueue){.front = 0, .back = -1, .size = 0};

  // Render - Texture - Atlas
  struct Atlas atlas = {
      .path = "assets/sprites/Effects/Fire/fire.png",
  };

  atlas.pixels = stbi_load(atlas.path, &atlas.w, &atlas.h, &atlas.channels, 0);
  if (!atlas.pixels) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Load image: %s\n",
                 stbi_failure_reason());
    return SDL_APP_FAILURE;
  }

  atlas.surface = SDL_CreateSurfaceFrom(
      atlas.w, atlas.h, SDL_PIXELFORMAT_RGBA8888, atlas.pixels, atlas.w * 4);
  if (!atlas.surface) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create surface: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  atlas.texture = SDL_CreateGPUTexture(
      render.device, &(SDL_GPUTextureCreateInfo){
                         .type = SDL_GPU_TEXTURETYPE_2D,
                         .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                         .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                         .width = atlas.w,
                         .height = atlas.h,
                         .layer_count_or_depth = 1,
                         .num_levels = 1,
                     });
  if (!atlas.texture) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create texture: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetGPUTextureName(render.device, atlas.texture, atlas.path);

  atlas.sampler = SDL_CreateGPUSampler(
      render.device,
      &(SDL_GPUSamplerCreateInfo){
          .min_filter = SDL_GPU_FILTER_NEAREST,
          .mag_filter = SDL_GPU_FILTER_NEAREST,
          .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
          .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
          .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      });
  if (!atlas.sampler) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create sampler: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  render.atlas = atlas;

  // GPU Buffers
  render.vertex = SDL_CreateGPUBuffer(
      render.device,
      &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                 .size = sizeof(struct Vertex) * MAX_VERTICES});
  if (!render.vertex) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create vertex buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetGPUBufferName(render.device, render.vertex, "Vertex");

  render.index = SDL_CreateGPUBuffer(
      render.device,
      &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                 .size = sizeof(uint16_t) * MAX_INDICES});
  if (!render.index) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create index buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetGPUBufferName(render.device, render.index, "Index");

  context->render = render;

  // Move static data --> GPU
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Transfer static GPU data");
  if (!EnqueueTransferToGPUBuffer(&context->render.queue, indices,
                                  sizeof(indices), context->render.index)) {
    return SDL_APP_FAILURE;
  }
  if (!EnqueueTransferToGPUTexture(
          &context->render.queue, context->render.atlas.pixels,
          AtlasSize(context->render.atlas), context->render.atlas.w,
          context->render.atlas.h, context->render.atlas.texture)) {
    return SDL_APP_FAILURE;
  }
  if (!EmptyGPUTransferQueue(&context->render.queue, context->render.device)) {
    return SDL_APP_FAILURE;
  }

  // TODO void loadDefaultGame();
  struct Game game = {
      .camera =
          {
              .position = {0.0, 0.0},
              .size = {(float)context->window.width /
                           (float)context->window.height,
                       1.0},
              .scale = 5.0,
          },
      .fire =
          {
              .position = {0.0, 0.0},
              .size = {(float)context->render.atlas.w /
                           (float)context->render.atlas.h,
                       1.0},
          },
  };
  context->game = game;

  *appstate = context;

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "App init complete");

  return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppIterate(void *appstate) {
  struct Context *context = (struct Context *)appstate;

  struct Vertex *vertices = SDL_malloc(MAX_VERTICES * sizeof(struct Vertex));
  size_t vertexCount = 0;

  // Game
  struct Fire fire = context->game.fire;

  context->game.camera.position[0] += 0.002;
  context->game.camera.position[1] += 0.002;

  mat4 mvp;
  camera_model_view_proj(context->game.camera, mvp);

  // Fire vertices
  vertices[vertexCount] = (struct Vertex){
      .position = {QUAD_VERTEX_POSITIONS[0] * fire.size[0] + fire.position[0],
                   QUAD_VERTEX_POSITIONS[1] * fire.size[1] + fire.position[1]},
      .uv = {QUAD_UV[0], QUAD_UV[1]},
  };
  vertexCount += 1;

  vertices[vertexCount] = (struct Vertex){
      .position = {QUAD_VERTEX_POSITIONS[2] * fire.size[0] + fire.position[0],
                   QUAD_VERTEX_POSITIONS[3] * fire.size[1] + fire.position[1]},
      .uv = {QUAD_UV[2], QUAD_UV[3]},
  };
  vertexCount += 1;

  vertices[vertexCount] = (struct Vertex){
      .position = {QUAD_VERTEX_POSITIONS[4] * fire.size[0] + fire.position[0],
                   QUAD_VERTEX_POSITIONS[5] * fire.size[1] + fire.position[1]},
      .uv = {QUAD_UV[4], QUAD_UV[5]},
  };
  vertexCount += 1;

  vertices[vertexCount] = (struct Vertex){
      .position = {QUAD_VERTEX_POSITIONS[6] * fire.size[0] + fire.position[0],
                   QUAD_VERTEX_POSITIONS[7] * fire.size[1] + fire.position[1]},
      .uv = {QUAD_UV[6], QUAD_UV[7]},
  };
  vertexCount += 1;
  assert(vertexCount % 4 == 0);
  SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "Dynamic vertex count: %zu",
               vertexCount);
  SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "%s\n",
               format_transformed_quad(&vertices[0], mvp));

  // Move dynamic data --> GPU
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Transfer dynamic GPU data");
  if (!EnqueueTransferToGPUBuffer(&context->render.queue, vertices,
                                  sizeof(struct Vertex) * vertexCount,
                                  context->render.vertex)) {
    return SDL_APP_FAILURE;
  }
  if (!EmptyGPUTransferQueue(&context->render.queue, context->render.device)) {
    return SDL_APP_FAILURE;
  }

  // Render - App render pass
  // NOTE: App render pass refers to a different context than an SDL render
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Render pass begin");
  SDL_GPUCommandBuffer *render =
      SDL_AcquireGPUCommandBuffer(context->render.device);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Acquired GPU command buffer");
  if (!render) {
    SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "Aqcuire command buffer: %s",
                SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // NOTE: Swapchain acquisition is a gate to begin a render pass, but our app
  // may continue successfully without producing a render pass on this
  // iteration.
  SDL_GPUTexture *swapchain;
  if (!SDL_AcquireGPUSwapchainTexture(render, context->window.window,
                                      &swapchain, NULL, NULL)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Acquire swapchain: %s",
                 SDL_GetError());
  }
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Acquired GPU swap chain texture");
  if (!swapchain) {
    SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "No swapchain texture acquired");
    return SDL_APP_CONTINUE;
  }

  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Push GPU uniform vertex data");
  SDL_PushGPUVertexUniformData(render, 0, mvp, sizeof(mvp));

  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "SDL render pass begin");
  SDL_GPURenderPass *pass =
      SDL_BeginGPURenderPass(render,
                             (SDL_GPUColorTargetInfo[]){{
                                 .texture = swapchain,
                                 .clear_color = CLEAR_COLOR,
                                 .load_op = SDL_GPU_LOADOP_CLEAR,
                                 .store_op = SDL_GPU_STOREOP_STORE,
                             }},
                             1, NULL);

  // Render - Binding
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Bind pipeline");
  SDL_BindGPUGraphicsPipeline(pass, context->render.pipeline);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Bind vertex buffer");
  SDL_BindGPUVertexBuffers(pass, 0,
                           (SDL_GPUBufferBinding[]){{
                               .buffer = context->render.vertex,
                           }},
                           1);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Bind index buffer");
  SDL_BindGPUIndexBuffer(
      pass, &(SDL_GPUBufferBinding){.buffer = context->render.index},
      INDEX_SIZE);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Bind atlas");
  assert(context->render.atlas.texture);
  assert(context->render.atlas.sampler);
  SDL_BindGPUFragmentSamplers(pass, 0,
                              (SDL_GPUTextureSamplerBinding[]){{
                                  .texture = context->render.atlas.texture,
                                  .sampler = context->render.atlas.sampler,
                              }},
                              1);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Binding complete");

  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Draw quads: %zu instances",
               vertexCount / 4);
  SDL_DrawGPUIndexedPrimitives(pass, QUAD_INDEX_COUNT, vertexCount / 4, 0, 0,
                               0);

  SDL_EndGPURenderPass(pass);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "SDL render pass complete");
  SDL_SubmitGPUCommandBuffer(render);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Render pass complete");

  SDL_free(vertices);

  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "App iterate complete");
  return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  struct Context *context = (struct Context *)appstate;

  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }
  if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
    return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
  SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "App event complete");
}
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  struct Context *context = (struct Context *)appstate;

  SDL_ReleaseGPUGraphicsPipeline(context->render.device,
                                 context->render.pipeline);
  SDL_ReleaseWindowFromGPUDevice(context->render.device,
                                 context->window.window);
  SDL_DestroyGPUDevice(context->render.device);
  SDL_DestroyWindow(context->window.window);
  SDL_free(context);

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "App quit complete");
}
