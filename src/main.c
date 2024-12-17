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

#include "cglm/types.h"

#include <stddef.h>
#include <stdint.h>

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
  SDL_GPUShaderFormat supportedFormats;
  SDL_GPUShaderFormat shaderFormat;
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
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
  SDL_Window *sdlWindow =
      SDL_CreateWindow(window.title, window.width, window.height, window.flags);
  if (!sdlWindow) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create window: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  window.window = sdlWindow;
  context->window = window;

  // Render
  struct Render render = {
      .supportedFormats = SDL_GPU_SHADERFORMAT_MSL,
  };
  SDL_GPUDevice *device =
      SDL_CreateGPUDevice(render.supportedFormats, true, NULL);
  if (!device) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create GPU device: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }
  render.device = device;

  if (!SDL_ClaimWindowForGPUDevice(render.device, sdlWindow)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Claim window for GPU: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUShaderFormat deviceFormats = SDL_GetGPUShaderFormats(render.device);

  const char *shaderPath;
  if (deviceFormats & SDL_GPU_SHADERFORMAT_MSL) {
    render.shaderFormat = SDL_GPU_SHADERFORMAT_MSL;
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
      .format = render.shaderFormat,
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
      .format = render.shaderFormat,
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
                          .src_color_blendfactor =
                              SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                          .dst_color_blendfactor =
                              SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                          .color_blend_op = SDL_GPU_BLENDOP_ADD,
                          .src_alpha_blendfactor =
                              SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                          .dst_alpha_blendfactor =
                              SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                          .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                          .enable_blend = true,
                      },
              }},
              .num_color_targets = 1,
          },
  };
  SDL_GPUGraphicsPipeline *pipeline =
      SDL_CreateGPUGraphicsPipeline(render.device, &pipelineInfo);
  if (!pipeline) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Create pipeline: %s\n",
                 SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_ReleaseGPUShader(render.device, vertex);
  SDL_ReleaseGPUShader(render.device, fragment);

  context->render = render;

  *appstate = context;
  return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppIterate(void *appstate) {
  struct Context *context = (struct Context *)appstate;

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
