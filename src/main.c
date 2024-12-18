#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_surface.h"
#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "cglm/types.h"

#include <stddef.h>
#include <stdint.h>

#define MAX_QUADS 1024
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)

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

struct Render {
  SDL_GPUShaderFormat supported;
  SDL_GPUShaderFormat shader;
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUTexture *atlas;
  SDL_GPUSampler *sampler;
  SDL_GPUBuffer *vertex;
  SDL_GPUBuffer *index;
};

struct Context {
  struct App app;
  struct Window window;
  struct Render render;
};

struct Vertex {
  vec2 position;
  vec2 uv;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
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
      .num_samplers = 1,
      .num_storage_textures = 1,
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
      .num_storage_textures = 1,
      .num_uniform_buffers = 1,
  };
  SDL_GPUShader *fragment = SDL_CreateGPUShader(render.device, &fragmentInfo);
  if (!fragment) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create fragment shader: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_free(contents);

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

  const char *atlasPath =
      "assets/sprites/Factions/Knights/Buildings/Castle/Castle_Blue.png";
  int w, h, channels;
  void *pixels = stbi_load(atlasPath, &w, &h, &channels, 0);
  if (!pixels) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Load image: %s\n",
                 stbi_failure_reason());
    return SDL_APP_FAILURE;
  }

  SDL_Surface *surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA8888,
                                               pixels, w * channels);
  if (!surface) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create surface: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  render.atlas = SDL_CreateGPUTexture(
      render.device, &(SDL_GPUTextureCreateInfo){
                         .type = SDL_GPU_TEXTURETYPE_2D,
                         .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                         .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                         .width = surface->w,
                         .height = surface->h,
                         .layer_count_or_depth = 1,
                         .num_levels = 1,
                     });
  if (!render.atlas) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create texture: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetGPUTextureName(render.device, render.atlas, "atlas");

  render.sampler = SDL_CreateGPUSampler(
      render.device,
      &(SDL_GPUSamplerCreateInfo){
          .min_filter = SDL_GPU_FILTER_NEAREST,
          .mag_filter = SDL_GPU_FILTER_NEAREST,
          .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
          .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
          .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      });
  if (!render.sampler) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create sampler: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  render.vertex = SDL_CreateGPUBuffer(
      render.device,
      &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                 .size = sizeof(struct Vertex) * MAX_VERTICES});
  if (!render.vertex) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create vertex buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  render.index = SDL_CreateGPUBuffer(
      render.device,
      &(SDL_GPUBufferCreateInfo){.usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                 .size = sizeof(uint16_t) * MAX_INDICES});
  if (!render.index) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create index buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Index data for quads can be precomputed
  uint16_t indices[MAX_INDICES];
  for (size_t i = 0; i < MAX_INDICES; i += 6) {
    // Vertex offset pattern to draw a quad
    // { 0, 1, 2, 0, 2, 3 }
    indices[i + 0] = i / 6 * 4 + 0;
    indices[i + 1] = i / 6 * 4 + 1;
    indices[i + 2] = i / 6 * 4 + 2;
    indices[i + 3] = i / 6 * 4 + 0;
    indices[i + 4] = i / 6 * 4 + 2;
    indices[i + 5] = i / 6 * 4 + 3;
  }

  SDL_GPUTransferBuffer *indexTransferBuffer = SDL_CreateGPUTransferBuffer(
      render.device, &(SDL_GPUTransferBufferCreateInfo){
                         .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                         .size = sizeof(indices)});
  if (!indexTransferBuffer) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create transfer buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  void *indexTransfer =
      SDL_MapGPUTransferBuffer(render.device, indexTransferBuffer, false);
  if (!indexTransfer) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Map transfer buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_memcpy(indexTransfer, &indices, sizeof(indices));
  SDL_UnmapGPUTransferBuffer(render.device, indexTransferBuffer);

  SDL_GPUTransferBuffer *textureTransferBuffer = SDL_CreateGPUTransferBuffer(
      render.device, &(SDL_GPUTransferBufferCreateInfo){
                         .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                         .size = surface->w * surface->h * 4});
  if (!textureTransferBuffer) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create transfer buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  void *textureTransfer =
      SDL_MapGPUTransferBuffer(render.device, textureTransferBuffer, false);
  if (!textureTransfer) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Map transfer buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_memcpy(textureTransfer, &surface->pixels, surface->w * surface->h * 4);
  SDL_UnmapGPUTransferBuffer(render.device, textureTransferBuffer);

  SDL_GPUCommandBuffer *upload = SDL_AcquireGPUCommandBuffer(render.device);
  if (!upload) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Acquire GPU command buffer: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(upload);
  SDL_UploadToGPUBuffer(
      pass,
      &(SDL_GPUTransferBufferLocation){.transfer_buffer = indexTransferBuffer},
      &(SDL_GPUBufferRegion){
          .buffer = render.index,
          .size = sizeof(indices),
      },
      false);
  SDL_UploadToGPUTexture(pass,
                         &(SDL_GPUTextureTransferInfo){
                             .transfer_buffer = textureTransferBuffer,
                             .pixels_per_row = surface->w,
                             .rows_per_layer = surface->h,
                         },
                         &(SDL_GPUTextureRegion){
                             .texture = render.atlas,
                             .mip_level = 1,
                             .layer = 1,
                             .w = surface->w,
                             .h = surface->h,
                             .d = 1,
                         },
                         false);
  SDL_EndGPUCopyPass(pass);
  SDL_SubmitGPUCommandBuffer(upload);

  SDL_ReleaseGPUTransferBuffer(render.device, indexTransferBuffer);

  context->render = render;

  *appstate = context;
  return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppIterate(void *appstate) {
  struct Context *context = (struct Context *)appstate;

  // Update

  // Draw

  // Bind vertex and index buffers

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
}
