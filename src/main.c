#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_scancode.h"
#include <assert.h>
#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"

// clang-format off
#include "cglm/affine.h"
#include "cglm/affine-pre.h" // clang-format on
#include "cglm/cam.h"
#include "cglm/mat4.h"
#include "cglm/vec2.h"
#include "cglm/types.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "atlas.h"
#include "physics.h"
#include "component.h"
#include "entity.h"
#include "markov.h"
#include "gen/atlas/resources.atlas.h"

#define UPDATES_PER_SECOND 60
#define SECONDS_PER_UPDATE (1.0 / UPDATES_PER_SECOND)
#define DELTA_NS (1000000000ULL / UPDATES_PER_SECOND)

#define MAX_QUADS 1024
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)
#define GPU_TRANSFER_QUEUE_MAX 8

#define CLEAR_COLOR (SDL_FColor){0.54, 0.9, 0.64, 1.0f}
#define INDEX_SIZE SDL_GPU_INDEXELEMENTSIZE_16BIT

#define QUAD_VERTEX_COUNT 4
#define QUAD_INDEX_COUNT 6
static float QUAD_VERTEX_POSITIONS[] = {-0.5, 0.5,  -0.5, -0.5,
                                        0.5,  -0.5, 0.5,  0.5};

#define MAX_SHEEP 1024

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
void AtlasCoordToUV(struct Atlas self, uint32_t x, uint32_t y, vec2 uv) {
  uv[0] = (float)x / (float)self.w;
  uv[1] = (float)y / (float)self.h;
}

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
  vec2 position; // Camera center in world space
  vec2 size;     // Viewport size in pixels
  float scale;   // world units per pixel
};
void camera_model_view_proj(struct Camera camera, mat4 mvp) {
  float scaled_width = camera.size[0] * camera.scale / 2;
  float scaled_height = camera.size[1] * camera.scale / 2;

  mat4 projection;
  glm_ortho(-scaled_width, scaled_width, -scaled_height, scaled_height, 0.0,
            1000.0, projection);

  mat4 view;
  glm_mat4_identity(view);
  glm_translate(view, (vec3){-camera.position[0], -camera.position[1], 0.0});

  glm_mat4_mul(projection, view, mvp);
}

struct Input {
  const bool *state;
  bool keys_pressed[SDL_SCANCODE_COUNT];
  bool keys_released[SDL_SCANCODE_COUNT];
};

struct AppTime {
  uint64_t currNs;
  uint64_t prevNs;
};

struct GameTime {
  uint64_t current;
  uint64_t accumulator;
};

/*struct Sheep {*/
/*  Kinematic kinematic;*/
/*  Collider collider;*/
/*  struct SpriteAnimation animation;*/
/*};*/

struct Game {
  struct GameTime time;
  struct Camera camera;
  Sheep sheep[MAX_SHEEP];
  size_t sheepCount;
  float sheepMaxSpeed;
  float sheepMaxDist;
  uint32_t spawnCooldown;
  float cameraSpeed;
};

struct Context {
  struct App app;
  struct Window window;
  struct Render render;
  struct Input input;
  struct Game game;
  struct AppTime time;
};

struct Vertex {
  vec2 position;
  vec2 uv;
};

struct QuadBuffer {
  struct Vertex vertices[MAX_VERTICES];
  size_t count;
};
size_t QuadBufferSize(struct QuadBuffer *self) {
  return sizeof(struct Vertex) * QUAD_VERTEX_COUNT * self->count;
}
bool QuadBufferAppend(struct QuadBuffer *quadBuf, vec2 quadPos, vec2 quadSize,
                      vec2 uvPos, vec2 uvSize) {
  if (quadBuf->count + 1 > MAX_QUADS) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Append quad: Quad buffer is full\n");
    return false;
  }

  struct Vertex v0 = {
      // FIXME: This is not correctly scaling the quad vertices. Making quads
      // too small because multiplying the size by 0.5
      .position = {QUAD_VERTEX_POSITIONS[0] * quadSize[0] + quadPos[0],
                   QUAD_VERTEX_POSITIONS[1] * quadSize[1] + quadPos[1]},
      .uv = {uvPos[0], uvPos[1]},
  };
  struct Vertex v1 = {
      .position = {QUAD_VERTEX_POSITIONS[2] * quadSize[0] + quadPos[0],
                   QUAD_VERTEX_POSITIONS[3] * quadSize[1] + quadPos[1]},
      .uv = {uvPos[0], uvPos[1] + uvSize[1]},
  };
  struct Vertex v2 = {
      .position = {QUAD_VERTEX_POSITIONS[4] * quadSize[0] + quadPos[0],
                   QUAD_VERTEX_POSITIONS[5] * quadSize[1] + quadPos[1]},
      .uv = {uvPos[0] + uvSize[0], uvPos[1] + uvSize[1]},
  };
  struct Vertex v3 = {
      .position = {QUAD_VERTEX_POSITIONS[6] * quadSize[0] + quadPos[0],
                   QUAD_VERTEX_POSITIONS[7] * quadSize[1] + quadPos[1]},
      .uv = {uvPos[0] + uvSize[0], uvPos[1]},
  };

  quadBuf->vertices[quadBuf->count * QUAD_VERTEX_COUNT + 0] = v0;
  quadBuf->vertices[quadBuf->count * QUAD_VERTEX_COUNT + 1] = v1;
  quadBuf->vertices[quadBuf->count * QUAD_VERTEX_COUNT + 2] = v2;
  quadBuf->vertices[quadBuf->count * QUAD_VERTEX_COUNT + 3] = v3;
  quadBuf->count += 1;

  return true;
}

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
    indices[index + 0] = index / QUAD_INDEX_COUNT * QUAD_VERTEX_COUNT + 0;
    indices[index + 1] = index / QUAD_INDEX_COUNT * QUAD_VERTEX_COUNT + 1;
    indices[index + 2] = index / QUAD_INDEX_COUNT * QUAD_VERTEX_COUNT + 2;
    indices[index + 3] = index / QUAD_INDEX_COUNT * QUAD_VERTEX_COUNT + 0;
    indices[index + 4] = index / QUAD_INDEX_COUNT * QUAD_VERTEX_COUNT + 2;
    indices[index + 5] = index / QUAD_INDEX_COUNT * QUAD_VERTEX_COUNT + 3;
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
    shaderPath = "assets/gen/shaders/metal/sprite.metal";
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
      .path = "assets/gen/atlas/resources.png",
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

  // Game - Init
  Sheep sheep = Sheep_Init();
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "state count: %zu\n",
               sheep.model->state_count);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "current state: %zu\n",
               sheep.model->current_state->id);

  struct Game game = {
      .time = {.current = 0},
      .camera =
          {
              .position = {0.0, 0.0},
              .size = {(float)context->window.width /
                           (float)context->window.height,
                       1.0},
              .scale = 5.0,
          },
      .sheep = {sheep},
      .sheepCount = 1,
      .sheepMaxSpeed = 20.0 * SECONDS_PER_UPDATE,
      .sheepMaxDist = 1.2,
      .spawnCooldown = 0,
      .cameraSpeed = 3.6 * SECONDS_PER_UPDATE,
  };
  context->game = game;

  context->time = (struct AppTime){
      .currNs = SDL_GetTicksNS(),
      .prevNs = SDL_GetTicksNS(),
  };

  context->input = (struct Input){0};

  *appstate = context;

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "App init complete");
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  struct Context *context = (struct Context *)appstate;

  struct QuadBuffer quadBuf = {0};

  context->input.state = SDL_GetKeyboardState(NULL);

  // App - Time
  context->time.currNs = SDL_GetTicksNS();
  uint64_t elapsed_ns = context->time.currNs - context->time.prevNs;
  context->time.prevNs = context->time.currNs;
  SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION,
               "App time: current: %llu elapsed: %llu", context->time.currNs,
               elapsed_ns);

  // Game - Time
  context->game.time.accumulator += elapsed_ns;
  uint64_t num_ticks = (uint64_t)(context->game.time.accumulator / DELTA_NS);
  context->game.time.accumulator -= num_ticks * DELTA_NS;
  SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION,
               "Game time: current: %llu accumulated: %llu",
               context->game.time.current, context->game.time.accumulator);

  // Game - Update
  for (size_t tick = 0; tick < num_ticks; tick++) {
    context->game.time.current += 1;

    // Animation step
    for (size_t sheepIdx = 0; sheepIdx < context->game.sheepCount; sheepIdx++) {
      Sheep *sheep = &context->game.sheep[sheepIdx];
      size_t state = get_current_state_id(sheep->model);
      SpriteDynamic *anim = SpriteMap_GetSprite(sheep->animations, state);
      SpritePlayback_Step(&sheep->playback, *anim);
    }

    // Camera movement
    vec2 moveDir = {0.0, 0.0};
    if (context->input.state[SDL_SCANCODE_W]) {
      moveDir[1] += context->game.cameraSpeed;
    }
    if (context->input.state[SDL_SCANCODE_A]) {
      moveDir[0] -= context->game.cameraSpeed;
    }
    if (context->input.state[SDL_SCANCODE_S]) {
      moveDir[1] -= context->game.cameraSpeed;
    }
    if (context->input.state[SDL_SCANCODE_D]) {
      moveDir[0] += context->game.cameraSpeed;
    }
    if (glm_vec2_norm(moveDir) > context->game.cameraSpeed) {
      glm_vec2_scale(
          moveDir, context->game.cameraSpeed / glm_vec2_norm(moveDir), moveDir);
    }
    context->game.camera.position[0] += moveDir[0];
    context->game.camera.position[1] += moveDir[1];

    /*// Spawn new sheep*/
    /*if (context->game.spawnCooldown > 0) {*/
    /*  context->game.spawnCooldown -= 1;*/
    /*}*/
    /*if (context->input.state[SDL_SCANCODE_SPACE] &&*/
    /*    context->game.sheepCount < MAX_SHEEP &&*/
    /*    context->game.spawnCooldown == 0) {*/
    /**/
    /*  context->game.sheep[context->game.sheepCount] = Sheep_Init();*/
    /*  context->game.sheepCount += 1;*/
    /*  context->game.spawnCooldown = 20;*/
    /*}*/
    /**/
    /*// Sheep movement*/
    /*for (size_t sheepIdx = 0; sheepIdx < context->game.sheepCount; sheepIdx++)
     * {*/
    /*  vec2 diff;*/
    /*  glm_vec2_sub(context->game.camera.position,*/
    /*               context->game.sheep[sheepIdx].kinematic.pos.raw, diff);*/
    /*  float dist = glm_vec2_norm2(diff);*/
    /*  /*if (dist > context->game.sheepMaxDist) {*/
    /*  context->game.sheep[sheepIdx].kinematic.vel.raw[0] = diff[0];*/
    /*  context->game.sheep[sheepIdx].kinematic.vel.raw[1] = diff[1];*/
    /*  /*}*/
    /*}*/
    /**/
    // Sheep collision
    /*for (size_t sheepIdx0 = 0; sheepIdx0 < context->game.sheepCount;*/
    /*     sheepIdx0++) {*/
    /*  for (size_t sheepIdx1 = sheepIdx0 + 1;*/
    /*       sheepIdx1 < context->game.sheepCount; sheepIdx1++) {*/
    /*    vec2 diff;*/
    /*    glm_vec2_sub(context->game.sheep[sheepIdx0].kinematic.pos.raw,*/
    /*                 context->game.sheep[sheepIdx1].kinematic.pos.raw,
     * diff);*/
    /**/
    /*    float dist = glm_vec2_norm(diff);*/
    /*    float overlap = context->game.sheep[sheepIdx0].collider.circle.radius
     * +*/
    /*                    context->game.sheep[sheepIdx1].collider.circle.radius
     * -*/
    /*                    dist;*/
    /*    if (overlap <= 0) {*/
    /*      continue;*/
    /*    }*/
    /**/
    /*    glm_vec2_normalize(diff);*/
    /*    glm_vec2_scale(diff, overlap, diff);*/
    /*    apply_force(&context->game.sheep[sheepIdx0].kinematic,*/
    /*                (vec2s){{diff[0], diff[1]}});*/
    /**/
    /*    glm_vec2_negate(diff);*/
    /*    apply_force(&context->game.sheep[sheepIdx1].kinematic,*/
    /*                (vec2s){{diff[0], diff[1]}});*/
    /*  }*/
    /*}*/
    /**/
    /*for (size_t sheepIdx = 0; sheepIdx < context->game.sheepCount; sheepIdx++)
     * {*/
    /*  integrate_velocity(&context->game.sheep[sheepIdx].kinematic,*/
    /*                     SECONDS_PER_UPDATE);*/
    /**/
    // Cap Sheep speed
    /*float speed =*/
    /*    glm_vec2_norm2(context->game.sheep[sheepIdx].kinematic.vel.raw);*/
    /*if (speed > context->game.sheepMaxSpeed) {*/
    /*  glm_vec2_norm(context->game.sheep[sheepIdx].kinematic.vel.raw);*/
    /*  glm_vec2_scale(context->game.sheep[sheepIdx].kinematic.vel.raw,*/
    /*                 context->game.sheepMaxSpeed,*/
    /*                 context->game.sheep[sheepIdx].kinematic.vel.raw);*/
    /*}*/
    /**/
    /*integrate_position(&context->game.sheep[sheepIdx].kinematic,*/
    /*                   SECONDS_PER_UPDATE);*/
    /**/
    /*if (get_state(context->game.sheep[sheepIdx].model->current_state,*/
    /*              SHEEP_STATE_ID_IDLE)) {*/
    /*} else if
     * (get_state(context->game.sheep[sheepIdx].model->current_state,*/
    /*                     SHEEP_STATE_ID_FOLLOW_POINT)) {*/
    /*}*/
    /**/
    /*switch (context->game.sheep[sheepIdx].model.currentState.id) {}*/

    /*speed =
     * glm_vec2_norm2(context->game.sheep[sheepIdx].kinematic.vel.raw);*/
    /*if (speed > 0.1) {*/
    /*  // Switch to bouncing state*/
    /*  if (strcmp("Happy_Sheep Idle",*/
    /*             context->game.sheep[sheepIdx].animation.animation->name)
     * ==*/
    /*      0) {*/
    /*    context->game.sheep[sheepIdx].animation.animation =*/
    /*        &ANIM_HAPPY_SHEEP_BOUNCING;*/
    /*    context->game.sheep[sheepIdx].animation.currentFrame = 0;*/
    /*    context->game.sheep[sheepIdx].animation.frameTimeAccumulator = 0;*/
    /*  }*/
    /*} else {*/
    /*  // Switch to the idle state*/
    /*  if (strcmp("Happy_Sheep Bouncing",*/
    /*             context->game.sheep[sheepIdx].animation.animation->name)
     * ==*/
    /*      0) {*/
    /*    context->game.sheep[sheepIdx].animation.animation =*/
    /*        &ANIM_HAPPY_SHEEP_IDLE;*/
    /*    context->game.sheep[sheepIdx].animation.currentFrame = 0;*/
    /*    context->game.sheep[sheepIdx].animation.frameTimeAccumulator = 0;*/
    /*  }*/
    /*}*/
    /**/
    // Reset for next frame
    /*context->game.sheep[sheepIdx].kinematic.vel = glms_vec2_zero();*/
    /*context->game.sheep[sheepIdx].kinematic.acc = glms_vec2_zero();*/
    /*}*/
  }

  mat4 mvp;
  camera_model_view_proj(context->game.camera, mvp);


  // Draw Sheep
  for (size_t sheepIdx = 0; sheepIdx < context->game.sheepCount; sheepIdx++) {
    Sheep sheep = context->game.sheep[sheepIdx];

    size_t state = get_current_state_id(sheep.model);
    SpriteDynamic *anim = SpriteMap_GetSprite(sheep.animations, state);
    struct AtlasRect rect = SpritePlayback_CurrentRect(sheep.playback, *anim);


    vec2 uv0;
    vec2 uv1;
    vec2 uv2;
    vec2 uv3;

    AtlasCoordToUV(context->render.atlas, rect.x, rect.y, uv0);
    AtlasCoordToUV(context->render.atlas, rect.x, rect.y + rect.h, uv1);
    AtlasCoordToUV(context->render.atlas, rect.x + rect.w, rect.y + rect.h,
                   uv2);
    AtlasCoordToUV(context->render.atlas, rect.x + rect.w, rect.y, uv3);

    vec2 uvPos = {uv0[0], uv0[1]};
    vec2 uvSize = {uv3[0] - uv0[0], uv1[1] - uv0[1]};

    if (!QuadBufferAppend(
            &quadBuf,
            (vec2){
                context->game.sheep[sheepIdx].kinematic.pos.raw[0],
                context->game.sheep[sheepIdx].kinematic.pos.raw[1],
            },
            (vec2){
                context->game.sheep[sheepIdx].collider.circle.radius,
                context->game.sheep[sheepIdx].collider.circle.radius,
            },
            uvPos, uvSize)) {
      return SDL_APP_FAILURE;
    }
  }


  // Move dynamic data --> GPU
  if (quadBuf.count > 0) {
    SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Transfer dynamic GPU data");
    if (!EnqueueTransferToGPUBuffer(&context->render.queue, quadBuf.vertices,
                                    QuadBufferSize(&quadBuf),
                                    context->render.vertex)) {
      return SDL_APP_FAILURE;
    }
    if (!EmptyGPUTransferQueue(&context->render.queue,
                               context->render.device)) {
      return SDL_APP_FAILURE;
    }
  } else {
    SDL_LogDebug(SDL_LOG_CATEGORY_GPU,
                 "No dynamic dynamic GPU data to transfer");
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

  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "Draw quads: %zu quads", quadBuf.count);
  uint32_t numIndices = QUAD_INDEX_COUNT * quadBuf.count;
  SDL_DrawGPUIndexedPrimitives(pass, numIndices, 1, 0, 0, 0);

  SDL_EndGPURenderPass(pass);
  SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "SDL render pass complete");
  SDL_SubmitGPUCommandBuffer(render);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Render pass complete");

  context->input = (struct Input){0};

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

  if (event->type == SDL_EVENT_KEY_DOWN) {
    // Ignore key repeat events
    if (!event->key.repeat) {
      context->input.keys_pressed[event->key.scancode] = true;
    }
  }
  if (event->type == SDL_EVENT_KEY_UP) {
    context->input.keys_released[event->key.scancode] = true;
  }

  SDL_LogTrace(SDL_LOG_CATEGORY_APPLICATION, "App event complete");
  return SDL_APP_CONTINUE;
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
